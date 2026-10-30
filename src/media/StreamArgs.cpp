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

	// Appends the `-f fmt -i url` (and friends) for one video source.
	void appendVideoInput(QStringList& arguments, const StreamSource& source, const StreamSettings& settings)
	{
		switch (source.kind)
		{
		case StreamSource::Kind::Camera:
			arguments << QStringLiteral("-f") << QStringLiteral("dshow")
				<< QStringLiteral("-i") << QStringLiteral("video=%1").arg(source.name);
			break;
		case StreamSource::Kind::Desktop:
			arguments << QStringLiteral("-f") << QStringLiteral("gdigrab")
				<< QStringLiteral("-framerate") << desktopFrameRate(settings.fps)
				<< QStringLiteral("-i") << QStringLiteral("desktop");
			break;
		case StreamSource::Kind::Window:
			// Large probesize: window content can change abruptly between frames.
			arguments << QStringLiteral("-f") << QStringLiteral("gdigrab")
				<< QStringLiteral("-framerate") << desktopFrameRate(settings.fps)
				<< QStringLiteral("-probesize") << QStringLiteral("50000000")
				<< QStringLiteral("-i") << QStringLiteral("title=%1").arg(source.name);
			break;
		case StreamSource::Kind::LocalFile:
			// -re pushes the file in real time.
			arguments << QStringLiteral("-re")
				<< QStringLiteral("-i") << source.name;
			break;
		}
	}

	void appendAudioInput(QStringList& arguments, const QString& microphoneName)
	{
		arguments << QStringLiteral("-f") << QStringLiteral("dshow")
			<< QStringLiteral("-i") << QStringLiteral("audio=%1").arg(microphoneName);
	}

	// Builds the overlay filter chain for videoCount inputs. Input 0 is the
	// base; every following input is scaled down and overlaid near the
	// bottom-right corner. The watermark image (last input, when present)
	// goes to the top-left corner like the legacy behaviour. The composed
	// stream is labelled [vout].
	QString overlayFilter(int videoCount, bool watermark)
	{
		QStringList chain;
		QString base = QStringLiteral("0:v");
		for (int i = 1; i < videoCount; ++i)
		{
			const bool last = i == videoCount - 1;
			const bool isWatermark = watermark && last;
			const QString scaled = QStringLiteral("s%1").arg(i);
			const QString out = last
				? QStringLiteral("vout")
				: QStringLiteral("o%1").arg(i);
			if (isWatermark)
			{
				chain << QStringLiteral("[%1:v]scale=120:-1[%2];[%3][%4]overlay=x=5:y=5[%5]")
					.arg(QString::number(i), scaled, base, scaled, out);
			}
			else
			{
				chain << QStringLiteral("[%1:v]scale=480:-1[%2];[%3][%4]overlay=x=main_w-w-24:y=main_h-h-24[%5]")
					.arg(QString::number(i), scaled, base, scaled, out);
			}
			base = out;
		}
		return chain.join(QLatin1Char(';'));
	}
}

QStringList buildSceneArgs(const QList<StreamSource>& sources,
	const StreamSettings& settings, QString* error)
{
	if (error)
		error->clear();

	if (sources.isEmpty())
	{
		if (error)
			*error = QStringLiteral("no source");
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

	int videoInputs = 0;
	for (const StreamSource& source : sources)
	{
		if (source.kind == StreamSource::Kind::Camera && source.name.isEmpty())
		{
			if (error)
				*error = QStringLiteral("no camera");
			return {};
		}
		if (source.kind == StreamSource::Kind::Window && source.name.isEmpty())
		{
			if (error)
				*error = QStringLiteral("no window selected");
			return {};
		}
		if (source.kind == StreamSource::Kind::LocalFile && !QFileInfo::exists(source.name))
		{
			if (error)
				*error = QStringLiteral("file missing");
			return {};
		}
		appendVideoInput(arguments, source, settings);
		++videoInputs;
	}

	const bool useWatermark = settings.watermark
		&& !settings.watermarkFile.isEmpty()
		&& QFileInfo::exists(settings.watermarkFile);
	if (useWatermark)
		arguments << QStringLiteral("-i") << settings.watermarkFile;

	const bool hasMicrophone = !settings.microphoneName.isEmpty();
	const int audioInputIndex = videoInputs + (useWatermark ? 1 : 0);
	if (hasMicrophone)
		appendAudioInput(arguments, settings.microphoneName);

	QStringList mappings;
	const int totalVideoInputs = videoInputs + (useWatermark ? 1 : 0);
	if (totalVideoInputs > 1)
		mappings << QStringLiteral("-filter_complex") << overlayFilter(totalVideoInputs, useWatermark)
			<< QStringLiteral("-map") << QStringLiteral("[vout]");

	if (hasMicrophone)
		mappings << QStringLiteral("-map") << QStringLiteral("%1:a").arg(audioInputIndex);
	else if (!sources.isEmpty() && sources.front().kind == StreamSource::Kind::LocalFile)
		mappings << QStringLiteral("-map") << QStringLiteral("0:a?");

	arguments << mappings;

	arguments << QStringLiteral("-f") << QStringLiteral("flv");
	if (settings.encoder != QStringLiteral("Auto") && !settings.encoder.isEmpty())
		arguments << QStringLiteral("-c:v") << settings.encoder;
	if (settings.bitrate != QStringLiteral("Auto") && !settings.bitrate.isEmpty())
		arguments << QStringLiteral("-b:v") << settings.bitrate + QStringLiteral("k");
	if (settings.fps != QStringLiteral("Auto") && !settings.fps.isEmpty()
		&& sources.front().kind == StreamSource::Kind::Camera)
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
	// Real-time stats (duration/bitrate/fps) for the status bar come from
	// the progress key=value lines written to stdout.
	arguments << QStringLiteral("-progress") << QStringLiteral("pipe:1") << url;
	return arguments;
}
