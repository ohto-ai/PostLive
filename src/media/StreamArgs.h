#pragma once
// Pure functions building the ffmpeg.exe argument list for a broadcast
// session, shared by LivePlatform and the unit tests.
//
// The scene is composited in-process (ScenePreview) and streamed as a single
// BGRA rawvideo stream through the ffmpeg stdin pipe — OBS-style WYSIWYG:
// whatever the preview canvas shows is what goes live. Audio comes from the
// microphone (dshow) as a separate input.
#include <QSize>
#include <QString>
#include <QStringList>

struct StreamSettings
{
	QString server;          // RTMP base URL, /livehime is appended
	QString microphoneName;  // dshow audio device name
	QString size;            // "1280x720", must match the composited canvas
	QString encoder;         // "Auto" keeps ffmpeg's default
	QString bitrate;         // "Auto" or kbps
	QString fps;             // "Auto" or a number
};

// Builds the arguments for streaming the composited canvas. `includeAudio`
// adds the microphone input. Returns an empty list and fills `error` when
// the settings are not streamable.
QStringList buildStreamPipeArgs(const QSize& canvasSize, const StreamSettings& settings,
	bool includeAudio, QString* error = nullptr);
