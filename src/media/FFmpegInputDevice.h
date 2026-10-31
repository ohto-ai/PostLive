#pragma once
// Captured input source, described the same way the ffmpeg CLI sees it:
// { -f input_format, -i url } plus per-media-type stream URLs. Ported from
// PostLive.Re (PostLiveClient/ffmpeg/FFmpegInputDevice).
#include <array>
#include <mutex>
#include <string>
#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
}

struct FFmpegInputDevice
{
	constexpr static size_t MediaTypeMax = 5;
	const struct AVInputFormat* input_format = nullptr;
	std::string device_name{};
	std::string device_description{};
	std::vector<enum AVMediaType> media_types{};
	std::array<std::string, MediaTypeMax> streamUrls{};

	static void updateInputDeviceList();
	static std::vector<const FFmpegInputDevice*> filterVideoDevices();
	static std::vector<const FFmpegInputDevice*> filterAudioDevices();

	// Full-desktop gdigrab capture, or a single window by exact title
	// (gdigrab resolves the title internally; HWND input is not supported).
	static FFmpegInputDevice make_gdi_device(std::string title = "");
	static FFmpegInputDevice make_local_file_device(std::string file);

private:
	static inline std::vector<FFmpegInputDevice> devices;
	static inline std::mutex devicesMutex;
	static inline std::once_flag registerFlag;
};
