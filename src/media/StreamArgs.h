#pragma once
// Pure function building the ffmpeg.exe argument list for a stream session,
// shared by LivePlatform and the unit tests.
#include <QString>
#include <QStringList>

enum class CaptureSource
{
	Camera,
	Desktop,
	Window,
	LocalFile,
};

struct StreamSettings
{
	CaptureSource source{ CaptureSource::Camera };
	QString server;          // RTMP base URL, /livehime is appended
	QString cameraName;      // dshow video device name
	QString microphoneName;  // dshow audio device name
	QString windowTitle;     // Window source: exact window title
	QString filePath;        // LocalFile source
	QString size;            // "1280x720"
	QString encoder;         // "Auto" keeps ffmpeg's default
	QString bitrate;         // "Auto" or kbps
	QString fps;             // "Auto" or a number
	bool watermark{ false };
	QString watermarkFile;
};

// Returns the ffmpeg.exe arguments for the given settings. Returns an empty
// list and fills `error` when the settings are not streamable.
QStringList buildStreamArgs(const StreamSettings& settings, QString* error = nullptr);
