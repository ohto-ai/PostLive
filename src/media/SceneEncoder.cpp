#include "SceneEncoder.h"

#include "../core/Logging.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
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
	const QString& encoderName, int bitrateKbps)
{
	m_hlsDir = hlsDir;
	m_canvasSize = canvasSize;
	m_fps = fps > 0 ? fps : 30;
	m_encoderName = encoderName;
	m_bitrateKbps = bitrateKbps > 0 ? bitrateKbps : 2500;
	start(QThread::HighPriority);
}

void SceneEncoder::stop()
{
	requestInterruption();
	wait();
	shutdown();
}

void SceneEncoder::onCanvasReady(const QImage& canvas)
{
	QMutexLocker lock(&m_frameMutex);
	if (canvas.size() == m_canvasSize)
	{
		m_latestCanvas = canvas;
		m_hasFrame = true;
	}
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

	m_codecContext = avcodec_alloc_context3(codec);
	if (m_codecContext == nullptr)
		return false;
	m_codecContext->width = m_canvasSize.width();
	m_codecContext->height = m_canvasSize.height();
	m_codecContext->pix_fmt = AV_PIX_FMT_YUV420P;
	m_codecContext->time_base = AVRational{ 1, m_fps };
	m_codecContext->framerate = AVRational{ m_fps, 1 };
	m_codecContext->bit_rate = qint64(m_bitrateKbps) * 1000;
	m_codecContext->gop_size = m_fps * 2;
	if (avcodec_open2(m_codecContext, codec, nullptr) < 0)
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

	AVStream* stream = avformat_new_stream(m_formatContext, nullptr);
	if (stream == nullptr)
		return false;
	m_videoStreamIndex = stream->index;
	stream->time_base = AVRational{ 1, m_fps };
	if (avcodec_parameters_from_context(stream->codecpar, m_codecContext) < 0)
		return false;

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

	m_frame = av_frame_alloc();
	m_frame->format = AV_PIX_FMT_YUV420P;
	m_frame->width = m_canvasSize.width();
	m_frame->height = m_canvasSize.height();
	if (av_frame_get_buffer(m_frame, 0) < 0)
		return false;

	m_swsContext = sws_getContext(m_canvasSize.width(), m_canvasSize.height(),
		AV_PIX_FMT_BGRA, m_canvasSize.width(), m_canvasSize.height(),
		AV_PIX_FMT_YUV420P, SWS_BILINEAR, nullptr, nullptr, nullptr);
	if (m_swsContext == nullptr)
	{
		qCWarning(lcStream) << "encoder: sws_getContext failed";
		return false;
	}

	qCInfo(lcStream) << "scene encoder started:" << playlist << m_canvasSize
		<< "@" << m_fps << "fps codec" << codec->name;
	return true;
}

void SceneEncoder::shutdown()
{
	if (m_codecContext != nullptr)
	{
		avcodec_send_frame(m_codecContext, nullptr); // flush
		AVPacket* packet = av_packet_alloc();
		while (avcodec_receive_packet(m_codecContext, packet) >= 0)
		{
			packet->stream_index = m_videoStreamIndex;
			av_packet_rescale_ts(packet,
				AVRational{ 1, m_fps }, m_formatContext->streams[m_videoStreamIndex]->time_base);
			av_interleaved_write_frame(m_formatContext, packet);
		}
		avcodec_send_frame(m_codecContext, nullptr);
		av_packet_free(&packet);
	}
	if (m_formatContext != nullptr && m_formatContext->pb != nullptr)
		av_write_trailer(m_formatContext);
	if (m_swsContext) sws_freeContext(m_swsContext);
	if (m_frame) av_frame_free(&m_frame);
	if (m_codecContext) avcodec_free_context(&m_codecContext);
	if (m_formatContext)
	{
		if (!(m_formatContext->oformat->flags & AVFMT_NOFILE) && m_formatContext->pb)
			avio_closep(&m_formatContext->pb);
		avformat_free_context(m_formatContext);
	}
	m_swsContext = nullptr;
	m_frame = nullptr;
	m_codecContext = nullptr;
	m_formatContext = nullptr;
	m_videoStreamIndex = -1;
	m_frameIndex = 0;
	qCInfo(lcStream) << "scene encoder stopped";
}

void SceneEncoder::run()
{
	if (!openOutput())
	{
		shutdown();
		return;
	}

	const qint64 frameInterval = 1000 / m_fps;
	AVPacket* packet = av_packet_alloc();

	while (!isInterruptionRequested())
	{
		const qint64 tick = QDateTime::currentMSecsSinceEpoch();

		QImage canvas;
		{
			QMutexLocker lock(&m_frameMutex);
			canvas = m_latestCanvas;
			m_hasFrame = false;
		}
		if (!canvas.isNull())
		{
			// BGRA canvas -> YUV420P encoder frame.
			const uint8_t* source[] = { canvas.constBits() };
			const int stride[] = { int(canvas.bytesPerLine()) };
			sws_scale(m_swsContext, source, stride, 0, m_canvasSize.height(),
				m_frame->data, m_frame->linesize);
			m_frame->pts = m_frameIndex++;

			if (avcodec_send_frame(m_codecContext, m_frame) >= 0)
			{
				while (avcodec_receive_packet(m_codecContext, packet) >= 0)
				{
					packet->stream_index = m_videoStreamIndex;
					av_packet_rescale_ts(packet,
						AVRational{ 1, m_fps },
						m_formatContext->streams[m_videoStreamIndex]->time_base);
					av_interleaved_write_frame(m_formatContext, packet);
				}
			}
		}

		const qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - tick;
		if (elapsed < frameInterval)
			msleep(ulong(frameInterval - elapsed));
	}

	av_packet_free(&packet);
	shutdown();
}
