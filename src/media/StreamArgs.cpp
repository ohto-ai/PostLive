#include "StreamArgs.h"

#include <QFileInfo>
#include <QUrl>

namespace
{
	// gdigrab has no -framerate default suitable for streaming.
	QString desktopFrameRate(const QString& fps)
	{
		return fps == QStringLiteral("Auto") ? QStringLiteral("30") : fps;
	}
}

QStringList buildStreamArgs(const StreamSettings& settings, QString* error)
{
	if (error)
		error->clear();

	const QString server = settings.server.trimmed();
	const QUrl serverUrl(server);
	if (server.isEmpty() || !serverUrl.isValid()
		|| serverUrl.scheme().isEmpty() || serverUrl.host().isEmpty())
	{
		if (error)
			*error = QStringLiteral("invalid server url");
		return {};
	}

	QStringList arguments;
	arguments << QStringLiteral("-hide_banner")
		<< QStringLiteral("-loglevel") << QStringLiteral("warning")
		<< QStringLiteral("-nostats");

	// Video input. Watermark is inserted as a second input right after the
	// video source so the audio input stays the last -i (existing behaviour).
	switch (settings.source)
	{
	case CaptureSource::Camera:
		if (settings.cameraName.isEmpty())
		{
			if (error)
				*error = QStringLiteral("no camera");
			return {};
		}
		arguments << QStringLiteral("-f") << QStringLiteral("dshow")
			<< QStringLiteral("-i") << QStringLiteral("video=%1").arg(settings.cameraName);
		break;
	case CaptureSource::Desktop:
		arguments << QStringLiteral("-f") << QStringLiteral("gdigrab")
			<< QStringLiteral("-framerate") << desktopFrameRate(settings.fps)
			<< QStringLiteral("-i") << QStringLiteral("desktop");
		break;
	case CaptureSource::Window:
		if (settings.windowTitle.isEmpty())
		{
			if (error)
				*error = QStringLiteral("no window selected");
			return {};
		}
		// Large probesize: window content can change abruptly between frames.
		arguments << QStringLiteral("-f") << QStringLiteral("gdigrab")
			<< QStringLiteral("-framerate") << desktopFrameRate(settings.fps)
			<< QStringLiteral("-probesize") << QStringLiteral("50000000")
			<< QStringLiteral("-i") << QStringLiteral("title=%1").arg(settings.windowTitle);
		break;
	case CaptureSource::LocalFile:
		if (settings.filePath.isEmpty() || !QFileInfo::exists(settings.filePath))
		{
			if (error)
				*error = QStringLiteral("file missing");
			return {};
		}
		// -re pushes the file in real time; its own audio track is used.
		arguments << QStringLiteral("-re")
			<< QStringLiteral("-i") << settings.filePath;
		break;
	}

	// Watermark silently skipped when the file is missing; the caller is
	// responsible for surfacing that in the UI.
	const bool useWatermark = settings.watermark
		&& !settings.watermarkFile.isEmpty()
		&& QFileInfo::exists(settings.watermarkFile);

	if (useWatermark)
		arguments << QStringLiteral("-i") << settings.watermarkFile;

	if (settings.source != CaptureSource::LocalFile)
	{
		if (settings.microphoneName.isEmpty())
		{
			if (error)
				*error = QStringLiteral("no microphone");
			return {};
		}
		arguments << QStringLiteral("-f") << QStringLiteral("dshow")
			<< QStringLiteral("-i") << QStringLiteral("audio=%1").arg(settings.microphoneName);
	}

	if (useWatermark)
		arguments << QStringLiteral("-filter_complex") << QStringLiteral("overlay=5:5");

	arguments << QStringLiteral("-f") << QStringLiteral("flv");
	if (settings.encoder != QStringLiteral("Auto") && !settings.encoder.isEmpty())
		arguments << QStringLiteral("-c:v") << settings.encoder;
	if (settings.bitrate != QStringLiteral("Auto") && !settings.bitrate.isEmpty())
		arguments << QStringLiteral("-b:v") << settings.bitrate + QStringLiteral("k");
	if (settings.fps != QStringLiteral("Auto") && !settings.fps.isEmpty()
		&& settings.source == CaptureSource::Camera)
		arguments << QStringLiteral("-r") << settings.fps;
	arguments << QStringLiteral("-pix_fmt") << QStringLiteral("yuv420p");

	QString size = settings.size;
	size.replace(QChar(0x00D7), QLatin1Char('x')); // 1280×720 -> 1280x720
	if (!size.isEmpty())
		arguments << QStringLiteral("-s") << size;

	QString url = server;
	while (url.endsWith(QLatin1Char('/')))
		url.chop(1);
	url += QStringLiteral("/livehime");
	arguments << url;
	return arguments;
}
