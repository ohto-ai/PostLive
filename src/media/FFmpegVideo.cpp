#include "FFmpegVideo.h"

#include "FFmpegInputDevice.h"
#include "../core/Logging.h"

#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include <chrono>
#include <functional>
#include <thread>

namespace
{
	// Local scope-exit helper (replaces PostLive.Re's utils/scope_guard.hpp).
	struct ScopeExit
	{
		std::function<void()> f;
		~ScopeExit() { f(); }
	};

	AVPixelFormat convertDeprecatedFormat(enum AVPixelFormat format)
	{
		switch (format)
		{
		case AV_PIX_FMT_YUVJ411P: return AV_PIX_FMT_YUV411P;
		case AV_PIX_FMT_YUVJ420P: return AV_PIX_FMT_YUV420P;
		case AV_PIX_FMT_YUVJ422P: return AV_PIX_FMT_YUV422P;
		case AV_PIX_FMT_YUVJ444P: return AV_PIX_FMT_YUV444P;
		case AV_PIX_FMT_YUVJ440P: return AV_PIX_FMT_YUV440P;
		default: return format;
		}
	}
}

FFmpegVideo::FFmpegVideo(QObject* parent)
	: QThread{ parent }
{
}

FFmpegVideo::~FFmpegVideo()
{
	stop();
}

bool FFmpegVideo::setInputDevice(const FFmpegInputDevice* device)
{
	std::lock_guard lock{ inputDeviceMutex };
	if (device == nullptr || std::find(device->media_types.begin(), device->media_types.end(),
		AVMEDIA_TYPE_VIDEO) == device->media_types.end())
		return false;
	inputDevice = device;
	return true;
}

void FFmpegVideo::clean()
{
	avcodec_free_context(&inputVideoCodecContext);
	avformat_close_input(&inputFormatContext);
	av_dict_free(&inputOptions);
	inputFormatContext = nullptr;
	inputVideoCodecContext = nullptr;
	inputVideoStreamIndex = -1;
	inputVideoCodec = nullptr;
	img_ctx = nullptr;
}

int FFmpegVideo::readInterruptCallback(void* opaque)
{
	// Aborts any blocking avformat call when the thread is asked to stop —
	// otherwise a device held by another instance hangs stop() forever.
	return static_cast<FFmpegVideo*>(opaque)->isInterruptionRequested() ? 1 : 0;
}

bool FFmpegVideo::open()
{
	do
	{
		if (inputDevice == nullptr)
			break;

		int ec = 0;
		inputFormatContext = avformat_alloc_context();
		inputFormatContext->interrupt_callback.callback = &FFmpegVideo::readInterruptCallback;
		inputFormatContext->interrupt_callback.opaque = this;
		ec = avformat_open_input(&inputFormatContext,
			inputDevice->streamUrls[AVMEDIA_TYPE_VIDEO].c_str(), inputDevice->input_format, &inputOptions);
		if (ec < 0)
		{
			postFFmpegError(ec);
			break;
		}

		ec = avformat_find_stream_info(inputFormatContext, &inputOptions);
		if (ec < 0)
		{
			postFFmpegError(ec);
			break;
		}

		inputVideoStreamIndex = av_find_best_stream(inputFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &inputVideoCodec, 0);
		if (inputVideoStreamIndex < 0)
		{
			postFFmpegError(inputVideoStreamIndex);
			break;
		}
		if (inputVideoCodec == nullptr)
		{
			emit postError(-1, QStringLiteral("Cannot find decoder."));
			break;
		}

		inputVideoCodecContext = avcodec_alloc_context3(inputVideoCodec);
		if (inputVideoCodecContext == nullptr)
		{
			emit postError(-1, QStringLiteral("Cannot allocate codec context."));
			break;
		}
		ec = avcodec_parameters_to_context(inputVideoCodecContext,
			inputFormatContext->streams[inputVideoStreamIndex]->codecpar);
		if (ec < 0)
		{
			postFFmpegError(ec);
			break;
		}

		ec = avcodec_open2(inputVideoCodecContext, inputVideoCodec, nullptr);
		if (ec < 0)
		{
			postFFmpegError(ec);
			break;
		}

		// Streams without known dimensions (still images, broken files) have
		// no sws-capable geometry: reject them instead of tripping swscale's
		// assertions.
		if (inputVideoCodecContext->width <= 0 || inputVideoCodecContext->height <= 0)
		{
			emit postError(-1, QStringLiteral("source has no video dimensions"));
			break;
		}

		const int w = inputVideoCodecContext->width;
		const int h = inputVideoCodecContext->height;
		img_ctx = sws_getContext(
			w, h, convertDeprecatedFormat(inputVideoCodecContext->pix_fmt),
			w, h, AV_PIX_FMT_RGB32,
			SWS_BICUBIC, nullptr, nullptr, nullptr);
		if (img_ctx == nullptr)
		{
			emit postError(-1, QStringLiteral("Cannot create sws context."));
			break;
		}
		return true;
	} while (false);
	return false;
}

void FFmpegVideo::run()
{
	ScopeExit cleanup{ [this] { clean(); } };
	if (!open())
		return;

	const int w = inputVideoCodecContext->width;
	const int h = inputVideoCodecContext->height;

	AVPacket* packet = nullptr;
	uint8_t* out_buffer = nullptr;
	AVFrame* yuvFrame = nullptr;
	AVFrame* rgbFrame = nullptr;

	ScopeExit frameCleanup{ [&]
		{
			av_packet_free(&packet);
			av_frame_free(&yuvFrame);
			av_frame_free(&rgbFrame);
			av_free(out_buffer);
		} };

	packet = av_packet_alloc();

	const int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB32, w, h, 1);
	out_buffer = static_cast<uint8_t*>(av_malloc(numBytes));

	yuvFrame = av_frame_alloc();
	rgbFrame = av_frame_alloc();
	av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, out_buffer, AV_PIX_FMT_RGB32, w, h, 1);

	// Real-time sources pace themselves; pacing here keeps local file
	// playback at the file's frame rate.
	const auto target_frame_duration = std::chrono::milliseconds(
		static_cast<long>(1000 / av_q2d(inputFormatContext->streams[inputVideoStreamIndex]->r_frame_rate)));
	auto start_time = std::chrono::steady_clock::now();

	while (!isInterruptionRequested() && av_read_frame(inputFormatContext, packet) >= 0)
	{
		if (packet->stream_index == inputVideoStreamIndex)
		{
			if (avcodec_send_packet(inputVideoCodecContext, packet) >= 0)
			{
				while (avcodec_receive_frame(inputVideoCodecContext, yuvFrame) >= 0)
				{
					sws_scale(img_ctx,
						yuvFrame->data, yuvFrame->linesize, 0, h,
						rgbFrame->data, rgbFrame->linesize);

					const QImage img(out_buffer, w, h, QImage::Format_RGB32);
					emit frameReady(img.copy());

					const auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(
						std::chrono::steady_clock::now() - start_time);
					const auto sleep_duration = target_frame_duration - elapsed_time;
					if (sleep_duration.count() > 0)
						std::this_thread::sleep_for(sleep_duration);
					else
						qCWarning(lcStream) << "preview decode is behind (" << elapsed_time.count() << "ms/frame)";
					start_time = std::chrono::steady_clock::now();
				}
			}
		}
		av_packet_unref(packet);
	}
}

void FFmpegVideo::stop()
{
	requestInterruption();
	wait();
}

void FFmpegVideo::addOption(const QString& key, const QString& value)
{
	std::lock_guard lock{ inputDeviceMutex };
	av_dict_set(&inputOptions, key.toUtf8().constData(), value.toUtf8().constData(), 0);
}

void FFmpegVideo::postFFmpegError(int error)
{
	char err_msg[512];
	av_strerror(error, err_msg, sizeof(err_msg));
	qCWarning(lcStream) << "FFmpeg error:" << error << err_msg;
	emit postError(error, QString::fromUtf8(err_msg));
}
