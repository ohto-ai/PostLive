#include "MicrophoneCapture.h"

#include "../core/Logging.h"

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

namespace
{
	constexpr int OutSampleRate = 48000;
	constexpr int OutChannels = 2;
}

MicrophoneCapture::MicrophoneCapture(QString deviceName, QObject* parent)
	: QThread(parent)
	, m_deviceName(std::move(deviceName))
{
}

void MicrophoneCapture::stop()
{
	requestInterruption();
	wait();
}

void MicrophoneCapture::run()
{
	avdevice_register_all();

	const AVInputFormat* inputFormat = av_find_input_format("dshow");
	if (inputFormat == nullptr)
	{
		qCWarning(lcStream) << "microphone: dshow demuxer unavailable";
		return;
	}

	const QString url = QStringLiteral("audio=%1").arg(m_deviceName);
	AVFormatContext* context = nullptr;
	AVDictionary* options = nullptr;
	av_dict_set(&options, "audio_buffer_size", "50", 0); // ms, low latency
	if (avformat_open_input(&context, url.toUtf8().constData(), inputFormat, &options) < 0)
	{
		qCWarning(lcStream) << "microphone: failed to open" << m_deviceName;
		av_dict_free(&options);
		return;
	}
	av_dict_free(&options);

	if (avformat_find_stream_info(context, nullptr) < 0)
	{
		qCWarning(lcStream) << "microphone: no stream info";
		avformat_close_input(&context);
		return;
	}

	// Abort blocking reads on stop(): dshow never signals EOF on its own.
	context->interrupt_callback.callback = [](void* opaque) -> int
	{
		return static_cast<MicrophoneCapture*>(opaque)->isInterruptionRequested() ? 1 : 0;
	};
	context->interrupt_callback.opaque = this;

	int streamIndex = av_find_best_stream(context, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
	if (streamIndex < 0)
	{
		qCWarning(lcStream) << "microphone: no audio stream";
		avformat_close_input(&context);
		return;
	}

	const AVStream* stream = context->streams[streamIndex];
	const int inRate = stream->codecpar->sample_rate;
	const int inChannels = stream->codecpar->ch_layout.nb_channels;
	qCInfo(lcStream) << "microphone capture started:" << m_deviceName
		<< inRate << "Hz" << inChannels << "ch";

	// Resampler: s16 (any layout/rate) -> fltp 48kHz stereo.
	SwrContext* swr = nullptr;
	AVChannelLayout inLayout{};
	av_channel_layout_default(&inLayout, inChannels);
	AVChannelLayout outLayout{};
	av_channel_layout_default(&outLayout, OutChannels);
	if (swr_alloc_set_opts2(&swr, &outLayout, AV_SAMPLE_FMT_FLTP, OutSampleRate,
		&inLayout, (AVSampleFormat)stream->codecpar->format, inRate, 0, nullptr) < 0
		|| swr_init(swr) < 0)
	{
		qCWarning(lcStream) << "microphone: swr setup failed";
		av_channel_layout_uninit(&inLayout);
		av_channel_layout_uninit(&outLayout);
		avformat_close_input(&context);
		return;
	}
	av_channel_layout_uninit(&inLayout);
	av_channel_layout_uninit(&outLayout);

	AVPacket* packet = av_packet_alloc();
	uint8_t** converted = nullptr;
	int convertedSamples = 0;
	int convertedCapacity = 0;

	while (!isInterruptionRequested())
	{
		if (av_read_frame(context, packet) < 0)
			break; // device gone
		if (packet->stream_index != streamIndex || packet->size <= 0)
		{
			av_packet_unref(packet);
			continue;
		}

		// Decode-free path: dshow delivers raw pcm_s16le interleaved samples.
		const uint8_t* in[] = { packet->data };
		const int inSamples = packet->size / (inChannels * 2);
		const int outCapacity = int(inSamples * int64_t(OutSampleRate) / inRate) + 256;
		if (outCapacity > convertedCapacity)
		{
			av_freep(&converted);
			if (av_samples_alloc_array_and_samples(&converted, nullptr, OutChannels,
				outCapacity, AV_SAMPLE_FMT_FLTP, 0) < 0)
			{
				av_packet_unref(packet);
				break;
			}
			convertedCapacity = outCapacity;
		}

		const int resampled = swr_convert(swr, converted, outCapacity, in, inSamples);
		av_packet_unref(packet);
		if (resampled <= 0)
			continue;

		// Peak level over the emitted chunk (pre-resample s16 is fine for a meter).
		float peak = 0.0f;
		{
			const auto* pcm = reinterpret_cast<const int16_t*>(packet->data);
			const int count = packet->size / int(sizeof(int16_t));
			for (int i = 0; i < count; ++i)
			{
				const float v = qAbs(float(pcm[i])) / 32768.0f;
				if (v > peak)
					peak = v;
			}
		}
		emit audioLevel(peak);

		// De-interleave into the planar fltp layout the AAC encoder wants.
		QByteArray planar(int(resampled) * OutChannels * sizeof(float), Qt::Uninitialized);
		auto* left = reinterpret_cast<float*>(planar.data());
		auto* right = left + resampled;
		const auto* source = reinterpret_cast<const float*>(converted[0]);
		for (int i = 0; i < resampled; ++i)
		{
			left[i] = source[i * 2];
			right[i] = source[i * 2 + 1];
		}
		emit audioReady(planar, resampled);
	}

	if (converted)
		av_freep(&converted);
	av_packet_free(&packet);
	swr_free(&swr);
	avformat_close_input(&context);
	qCInfo(lcStream) << "microphone capture stopped";
}
