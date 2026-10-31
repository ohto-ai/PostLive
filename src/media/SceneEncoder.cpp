#include "SceneEncoder.h"

#include "MicrophoneCapture.h"
#include "../core/Logging.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

#include <QDateTime>
#include <QDir>

namespace
{
	void logAvError(const char* what, int error)
	{
		char message[256];
		av_strerror(error, message, sizeof(message));
		qCWarning(lcStream) << what << "failed:" << error << message;
	}
}

SceneEncoder::SceneEncoder(QObject* parent)
	: QThread(parent)
{
}

SceneEncoder::~SceneEncoder()
{
	stop();
}

void SceneEncoder::startHls(const QString& hlsDir, const QSize& canvasSize, int fps,
	const QString& encoderName, int bitrateKbps, bool withAudio,
	const QString& microphoneName)
{
	m_hlsDir = hlsDir;
	m_canvasSize = canvasSize;
	m_fps = fps > 0 ? fps : 30;
	m_encoderName = encoderName;
	m_bitrateKbps = bitrateKbps > 0 ? bitrateKbps : 2500;
	m_withAudio = withAudio;
	m_microphoneName = microphoneName;

	if (m_withAudio)
	{
		m_micCapture = new MicrophoneCapture(m_microphoneName, this);
		connect(m_micCapture, &MicrophoneCapture::audioReady,
			this, &SceneEncoder::onAudioReady);
		m_micCapture->start(QThread::HighPriority);
	}
	start(QThread::HighPriority);
}

void SceneEncoder::stop()
{
	requestInterruption();
	wait();
	if (m_micCapture != nullptr)
	{
		m_micCapture->stop();
		delete m_micCapture;
		m_micCapture = nullptr;
	}
	shutdown();
}

void SceneEncoder::onCanvasReady(const QImage& canvas)
{
	QMutexLocker lock(&m_frameMutex);
	if (canvas.size() == m_canvasSize)
		m_latestCanvas = canvas;
}

void SceneEncoder::onAudioReady(const QByteArray& samplesFltp, int sampleCount)
{
	QMutexLocker lock(&m_audioMutex);
	m_hasAudio = true;
	if (m_audioFifo == nullptr)
		return;
	// The capture delivers one contiguous planar buffer (left samples, then
	// right samples) which matches the fltp layout.
	void* planes[2] = {
		reinterpret_cast<uint8_t*>(const_cast<char*>(samplesFltp.constData())),
		reinterpret_cast<uint8_t*>(const_cast<char*>(samplesFltp.constData()))
			+ sampleCount * sizeof(float)
	};
	if (av_audio_fifo_write(m_audioFifo, planes, sampleCount) < sampleCount)
		qCWarning(lcStream) << "audio fifo overflow, samples dropped";
}

bool SceneEncoder::openAudioStream()
{
	const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
	if (codec == nullptr)
	{
		qCWarning(lcStream) << "encoder: no aac encoder in the FFmpeg runtime";
		return false;
	}

	m_audioCodecContext = avcodec_alloc_context3(codec);
	if (m_audioCodecContext == nullptr)
		return false;
	m_audioCodecContext->sample_rate = 48000;
	av_channel_layout_default(&m_audioCodecContext->ch_layout, 2);
	m_audioCodecContext->sample_fmt = AV_SAMPLE_FMT_FLTP;
	m_audioCodecContext->time_base = AVRational{ 1, 48000 };
	m_audioCodecContext->bit_rate = 128000;
	if (avcodec_open2(m_audioCodecContext, codec, nullptr) < 0)
	{
		logAvError("avcodec_open2(audio)", 0);
		return false;
	}

	AVStream* stream = avformat_new_stream(m_formatContext, nullptr);
	if (stream == nullptr)
		return false;
	m_audioStreamIndex = stream->index;
	stream->time_base = m_audioCodecContext->time_base;
	if (avcodec_parameters_from_context(stream->codecpar, m_audioCodecContext) < 0)
		return false;

	m_audioFifo = av_audio_fifo_alloc(AV_SAMPLE_FMT_FLTP, 2, 48000 * 4);
	return m_audioFifo != nullptr;
}

bool SceneEncoder::openOutput()
{
	const AVCodec* codec = m_encoderName.isEmpty() || m_encoderName == QLatin1String("Auto")
		? avcodec_find_encoder(AV_CODEC_ID_H264)
		: avcodec_find_encoder_by_name(m_encoderName.toUtf8().constData());
	if (codec == nullptr)
		codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (codec == nullptr)
	{
		qCWarning(lcStream) << "encoder: no h264 encoder in the FFmpeg runtime";
		return false;
	}

	m_videoCodecContext = avcodec_alloc_context3(codec);
	if (m_videoCodecContext == nullptr)
		return false;
	m_videoCodecContext->width = m_canvasSize.width();
	m_videoCodecContext->height = m_canvasSize.height();
	m_videoCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
	m_videoCodecContext->time_base = AVRational{ 1, m_fps };
	m_videoCodecContext->framerate = AVRational{ m_fps, 1 };
	m_videoCodecContext->bit_rate = qint64(m_bitrateKbps) * 1000;
	m_videoCodecContext->gop_size = m_fps * 2;
	if (avcodec_open2(m_videoCodecContext, codec, nullptr) < 0)
	{
		logAvError("avcodec_open2", 0);
		return false;
	}

	const QString playlist = m_hlsDir + QStringLiteral("/livehime.m3u8");
	const int formatError = avformat_alloc_output_context2(&m_formatContext,
		nullptr, nullptr, playlist.toUtf8().constData());
	if (formatError < 0 || m_formatContext == nullptr)
	{
		logAvError("avformat_alloc_output_context2", formatError);
		return false;
	}

	AVStream* videoStream = avformat_new_stream(m_formatContext, nullptr);
	if (videoStream == nullptr)
		return false;
	m_videoStreamIndex = videoStream->index;
	videoStream->time_base = AVRational{ 1, m_fps };
	if (avcodec_parameters_from_context(videoStream->codecpar, m_videoCodecContext) < 0)
		return false;

	if (m_withAudio && !openAudioStream())
	{
		qCWarning(lcStream) << "encoder: audio stream unavailable, continuing video-only";
		m_withAudio = false;
	}

	AVDictionary* muxerOptions = nullptr;
	av_dict_set(&muxerOptions, "hls_time", "2", 0);
	av_dict_set(&muxerOptions, "hls_list_size", "6", 0);
	av_dict_set(&muxerOptions, "hls_flags", "delete_segments+independent_segments+temp_file", 0);

	if (!(m_formatContext->oformat->flags & AVFMT_NOFILE))
	{
		const int openError = avio_open(&m_formatContext->pb,
			playlist.toUtf8().constData(), AVIO_FLAG_WRITE);
		if (openError < 0)
		{
			logAvError("avio_open", openError);
			av_dict_free(&muxerOptions);
			return false;
		}
	}

	const int headerError = avformat_write_header(m_formatContext, &muxerOptions);
	av_dict_free(&muxerOptions);
	if (headerError < 0)
	{
		logAvError("avformat_write_header", headerError);
		return false;
	}

	m_videoFrame = av_frame_alloc();
	m_videoFrame->format = AV_PIX_FMT_YUV420P;
	m_videoFrame->width = m_canvasSize.width();
	m_videoFrame->height = m_canvasSize.height();
	if (av_frame_get_buffer(m_videoFrame, 0) < 0)
		return false;

	if (m_withAudio)
	{
		m_audioFrame = av_frame_alloc();
		m_audioFrame->format = AV_SAMPLE_FMT_FLTP;
		m_audioFrame->ch_layout = m_audioCodecContext->ch_layout;
		m_audioFrame->sample_rate = 48000;
		m_audioFrame->nb_samples = m_audioCodecContext->frame_size;
		if (av_frame_get_buffer(m_audioFrame, 0) < 0)
			return false;
	}

	m_swsContext = sws_getContext(m_canvasSize.width(), m_canvasSize.height(),
		AV_PIX_FMT_BGRA, m_canvasSize.width(), m_canvasSize.height(),
		AV_PIX_FMT_YUV420P, SWS_BILINEAR, nullptr, nullptr, nullptr);
	if (m_swsContext == nullptr)
	{
		qCWarning(lcStream) << "encoder: sws_getContext failed";
		return false;
	}

	qCInfo(lcStream) << "scene encoder started:" << playlist << m_canvasSize
		<< "@" << m_fps << "fps codec" << codec->name
		<< "audio:" << (m_withAudio ? "aac" : "off");
	return true;
}

void SceneEncoder::drainAudioFifo()
{
	// Encode full AAC frames (frame_size samples each) from the fifo.
	while (m_withAudio && av_audio_fifo_size(m_audioFifo) >= m_audioFrame->nb_samples)
	{
		av_frame_make_writable(m_audioFrame);
		if (av_audio_fifo_read(m_audioFifo,
			reinterpret_cast<void**>(m_audioFrame->data), m_audioFrame->nb_samples)
			< m_audioFrame->nb_samples)
			return;
		m_audioFrame->pts = m_audioSampleIndex;
		m_audioSampleIndex += m_audioFrame->nb_samples;

		if (avcodec_send_frame(m_audioCodecContext, m_audioFrame) >= 0)
		{
			AVPacket* packet = av_packet_alloc();
			while (avcodec_receive_packet(m_audioCodecContext, packet) >= 0)
			{
				packet->stream_index = m_audioStreamIndex;
				av_packet_rescale_ts(packet, m_audioCodecContext->time_base,
					m_formatContext->streams[m_audioStreamIndex]->time_base);
				av_interleaved_write_frame(m_formatContext, packet);
			}
			av_packet_free(&packet);
		}
	}
}

void SceneEncoder::flushEncoders()
{
	AVPacket* packet = av_packet_alloc();
	if (m_videoCodecContext != nullptr)
	{
		avcodec_send_frame(m_videoCodecContext, nullptr);
		while (avcodec_receive_packet(m_videoCodecContext, packet) >= 0)
		{
			packet->stream_index = m_videoStreamIndex;
			av_packet_rescale_ts(packet, AVRational{ 1, m_fps },
				m_formatContext->streams[m_videoStreamIndex]->time_base);
			av_interleaved_write_frame(m_formatContext, packet);
		}
	}
	if (m_audioCodecContext != nullptr)
	{
		// Drain remaining full frames; never send more samples than the
		// frame buffer holds (an oversized frame overflows its data buffers).
		while (m_withAudio && av_audio_fifo_size(m_audioFifo) >= m_audioFrame->nb_samples)
		{
			av_frame_make_writable(m_audioFrame);
			if (av_audio_fifo_read(m_audioFifo,
				reinterpret_cast<void**>(m_audioFrame->data), m_audioFrame->nb_samples)
				== m_audioFrame->nb_samples)
			{
				m_audioFrame->pts = m_audioSampleIndex;
				m_audioSampleIndex += m_audioFrame->nb_samples;
				avcodec_send_frame(m_audioCodecContext, m_audioFrame);
			}
		}
		avcodec_send_frame(m_audioCodecContext, nullptr);
		while (avcodec_receive_packet(m_audioCodecContext, packet) >= 0)
		{
			packet->stream_index = m_audioStreamIndex;
			av_packet_rescale_ts(packet, m_audioCodecContext->time_base,
				m_formatContext->streams[m_audioStreamIndex]->time_base);
			av_interleaved_write_frame(m_formatContext, packet);
		}
	}
	av_packet_free(&packet);
}

void SceneEncoder::shutdown()
{
	if (m_videoCodecContext != nullptr || m_audioCodecContext != nullptr)
		flushEncoders();
	if (m_formatContext != nullptr && m_formatContext->pb != nullptr)
		av_write_trailer(m_formatContext);
	if (m_swsContext) sws_freeContext(m_swsContext);
	if (m_videoFrame) av_frame_free(&m_videoFrame);
	if (m_audioFrame) av_frame_free(&m_audioFrame);
	if (m_videoCodecContext) avcodec_free_context(&m_videoCodecContext);
	if (m_audioCodecContext) avcodec_free_context(&m_audioCodecContext);
	if (m_audioFifo) av_audio_fifo_free(m_audioFifo);
	if (m_formatContext)
	{
		if (!(m_formatContext->oformat->flags & AVFMT_NOFILE) && m_formatContext->pb)
			avio_closep(&m_formatContext->pb);
		avformat_free_context(m_formatContext);
	}
	m_swsContext = nullptr;
	m_videoFrame = m_audioFrame = nullptr;
	m_videoCodecContext = m_audioCodecContext = nullptr;
	m_audioFifo = nullptr;
	m_formatContext = nullptr;
	m_videoStreamIndex = m_audioStreamIndex = -1;
	m_frameIndex = 0;
	m_audioSampleIndex = 0;
	qCInfo(lcStream) << "scene encoder stopped";
}

void SceneEncoder::run()
{
	// The microphone thread starts first; give it a moment to deliver its
	// first samples before deciding whether the output carries an audio
	// stream (a dead device degrades to video-only instead of a stalled
	// muxer).
	if (m_withAudio)
	{
		const qint64 waitStart = QDateTime::currentMSecsSinceEpoch();
		while (!m_hasAudio && !isInterruptionRequested()
			&& QDateTime::currentMSecsSinceEpoch() - waitStart < 5000)
			msleep(20);
		if (!m_hasAudio)
		{
			qCWarning(lcStream) << "microphone delivered no audio in 5s, streaming video-only";
			m_withAudio = false;
		}
	}

	if (!openOutput())
	{
		shutdown();
		return;
	}

	const qint64 frameInterval = 1000 / m_fps;
	while (!isInterruptionRequested())
	{
		const qint64 tick = QDateTime::currentMSecsSinceEpoch();

		QImage canvas;
		{
			QMutexLocker lock(&m_frameMutex);
			canvas = m_latestCanvas;
		}
		if (!canvas.isNull())
		{
			// BGRA canvas -> YUV420P encoder frame. make_writable first: the
			// encoder may still hold a reference to the previous frame.
			av_frame_make_writable(m_videoFrame);
			const uint8_t* source[] = { canvas.constBits() };
			const int stride[] = { int(canvas.bytesPerLine()) };
			sws_scale(m_swsContext, source, stride, 0, m_canvasSize.height(),
				m_videoFrame->data, m_videoFrame->linesize);
			m_videoFrame->pts = m_frameIndex++;

			if (avcodec_send_frame(m_videoCodecContext, m_videoFrame) >= 0)
			{
				AVPacket* packet = av_packet_alloc();
				while (avcodec_receive_packet(m_videoCodecContext, packet) >= 0)
				{
					packet->stream_index = m_videoStreamIndex;
					av_packet_rescale_ts(packet,
						AVRational{ 1, m_fps },
						m_formatContext->streams[m_videoStreamIndex]->time_base);
					av_interleaved_write_frame(m_formatContext, packet);
				}
				av_packet_free(&packet);
			}

			drainAudioFifo();
		}

		const qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - tick;
		if (elapsed < frameInterval)
			msleep(ulong(frameInterval - elapsed));
	}

	shutdown();
}
