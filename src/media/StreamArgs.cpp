#include "StreamArgs.h"

#include <QUrl>

namespace
{
	QString effectiveFps(const QString& fps)
	{
		return (fps.isEmpty() || fps == QStringLiteral("Auto"))
			? QStringLiteral("30") : fps;
	}
}

QStringList buildStreamPipeArgs(const QSize& canvasSize, const StreamSettings& settings,
	bool includeAudio, QString* error)
{
	if (error)
		error->clear();

	if (canvasSize.isEmpty())
	{
		if (error)
			*error = QStringLiteral("canvas size not ready");
		return {};
	}

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

	// The composited canvas arrives as raw BGRA frames on stdin.
	arguments << QStringLiteral("-f") << QStringLiteral("rawvideo")
		<< QStringLiteral("-pix_fmt") << QStringLiteral("bgra")
		<< QStringLiteral("-s") << QString::number(canvasSize.width())
			+ QStringLiteral("x") + QString::number(canvasSize.height())
		<< QStringLiteral("-r") << effectiveFps(settings.fps)
		<< QStringLiteral("-i") << QStringLiteral("-");

	if (includeAudio)
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

	arguments << QStringLiteral("-f") << QStringLiteral("flv");
	if (settings.encoder != QStringLiteral("Auto") && !settings.encoder.isEmpty())
		arguments << QStringLiteral("-c:v") << settings.encoder;
	if (settings.bitrate != QStringLiteral("Auto") && !settings.bitrate.isEmpty())
		arguments << QStringLiteral("-b:v") << settings.bitrate + QStringLiteral("k");
	arguments << QStringLiteral("-pix_fmt") << QStringLiteral("yuv420p");

	QString url = server;
	while (url.endsWith(QLatin1Char('/')))
		url.chop(1);
	url += QStringLiteral("/livehime");
	// Real-time stats (duration/bitrate/fps) for the control dock come from
	// the progress key=value lines written to stdout.
	arguments << QStringLiteral("-progress") << QStringLiteral("pipe:1") << url;
	return arguments;
}
