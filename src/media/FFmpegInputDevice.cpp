#include "FFmpegInputDevice.h"

#include "../core/Logging.h"

#include <algorithm>

void FFmpegInputDevice::updateInputDeviceList()
{
#ifdef _WIN32
	const std::vector<std::string> inputDeviceFormats = { "dshow" };
#else
	const std::vector<std::string> inputDeviceFormats = {};
#endif
	std::call_once(registerFlag, avdevice_register_all);
	std::lock_guard lock{ devicesMutex };
	devices.clear();
	for (const auto& format : inputDeviceFormats)
	{
		const auto inputDeviceFormat = av_find_input_format(format.c_str());
		if (inputDeviceFormat == nullptr)
			continue;

		AVDeviceInfoList* deviceList = nullptr;
		if (avdevice_list_input_sources(inputDeviceFormat, nullptr, nullptr, &deviceList) < 0)
			continue;

		for (int i = 0; i < deviceList->nb_devices; i++)
		{
			FFmpegInputDevice device;
			device.input_format = inputDeviceFormat;
			device.device_name = deviceList->devices[i]->device_name;
			device.device_description = deviceList->devices[i]->device_description;
			for (int j = 0; j < deviceList->devices[i]->nb_media_types; j++)
			{
				const auto media_type = deviceList->devices[i]->media_types[j];
				device.media_types.push_back(media_type);
				if (media_type == AVMEDIA_TYPE_VIDEO)
					device.streamUrls[AVMEDIA_TYPE_VIDEO] = "video=" + device.device_name;
				else if (media_type == AVMEDIA_TYPE_AUDIO)
					device.streamUrls[AVMEDIA_TYPE_AUDIO] = "audio=" + device.device_name;
			}
			devices.push_back(std::move(device));
		}
		avdevice_free_list_devices(&deviceList);
	}
	qCDebug(lcDevice) << "enumerated" << devices.size() << "dshow devices";
}

std::vector<const FFmpegInputDevice*> FFmpegInputDevice::filterVideoDevices()
{
	std::lock_guard lock{ devicesMutex };
	std::vector<const FFmpegInputDevice*> result;
	for (const auto& device : devices)
	{
		if (std::find(device.media_types.begin(), device.media_types.end(), AVMEDIA_TYPE_VIDEO)
			!= device.media_types.end())
			result.push_back(&device);
	}
	return result;
}

std::vector<const FFmpegInputDevice*> FFmpegInputDevice::filterAudioDevices()
{
	std::lock_guard lock{ devicesMutex };
	std::vector<const FFmpegInputDevice*> result;
	for (const auto& device : devices)
	{
		if (std::find(device.media_types.begin(), device.media_types.end(), AVMEDIA_TYPE_AUDIO)
			!= device.media_types.end())
			result.push_back(&device);
	}
	return result;
}

FFmpegInputDevice FFmpegInputDevice::make_gdi_device(std::string title)
{
	FFmpegInputDevice device{};
	device.input_format = av_find_input_format("gdigrab");
	if (title.empty())
	{
		device.device_name = "desktop";
		device.device_description = "GDI Grab - Desktop";
		device.media_types.push_back(AVMEDIA_TYPE_VIDEO);
		device.streamUrls[AVMEDIA_TYPE_VIDEO] = "desktop";
	}
	else
	{
		device.device_name = title;
		device.device_description = "GDI Grab - \"" + title + "\"";
		device.media_types.push_back(AVMEDIA_TYPE_VIDEO);
		device.streamUrls[AVMEDIA_TYPE_VIDEO] = "title=" + title;
	}
	return device;
}

FFmpegInputDevice FFmpegInputDevice::make_local_file_device(std::string file)
{
	FFmpegInputDevice device{};
	device.input_format = nullptr; // demuxer auto-detection
	device.device_name = file;
	device.device_description = "Local File - " + file;
	device.media_types.push_back(AVMEDIA_TYPE_VIDEO);
	device.streamUrls[AVMEDIA_TYPE_VIDEO] = file;
	return device;
}
