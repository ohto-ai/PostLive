#pragma once
// Pure functions building the ffmpeg.exe argument list for a broadcast
// session, shared by LivePlatform and the unit tests.
#include <QString>
#include <QStringList>
#include "StreamSource.h"

struct StreamSettings
{
	QString server;          // RTMP base URL, /livehime is appended
	QString microphoneName;  // dshow audio device name ("virtual-audio-capturer" etc.)
	QString size;            // "1280x720"
	QString encoder;         // "Auto" keeps ffmpeg's default
	QString bitrate;         // "Auto" or kbps
	QString fps;             // "Auto" or a number
	bool watermark{ false };
	QString watermarkFile;
};

// Builds the arguments for the given scene (ordered sources: the first one
// is the base layer, the rest are overlaid bottom-right as picture-in-picture)
// and settings. Returns an empty list and fills `error` when the scene is not
// streamable. The watermark, when enabled and present, is added as the top
// layer automatically.
QStringList buildSceneArgs(const QList<StreamSource>& sources,
	const StreamSettings& settings, QString* error = nullptr);
