#include <QtTest>

#include "StreamArgs.h"

using CaptureSource = ::CaptureSource;

// Contract tests for the ffmpeg.exe argument builder.
class TestStreamArgs : public QObject
{
	Q_OBJECT

private slots:
	void cameraSourceBuildsDshowPipeline()
	{
		StreamSettings s;
		s.source = CaptureSource::Camera;
		s.server = "rtmp://rtmp.example.com";
		s.cameraName = "USB Camera";
		s.microphoneName = "virtual-audio-capturer";
		s.size = "1280x720";
		s.encoder = "libx264";
		s.bitrate = "2500";
		s.fps = "30";

		const QStringList args = buildStreamArgs(s);
		QVERIFY(!args.isEmpty());
		const QString joined = args.join(' ');
		QVERIFY(joined.contains("-f dshow"));
		QVERIFY(joined.contains("-i video=USB Camera"));
		QVERIFY(joined.contains("-i audio=virtual-audio-capturer"));
		QVERIFY(joined.contains("-c:v libx264"));
		QVERIFY(joined.contains("-b:v 2500k"));
		QVERIFY(joined.contains("-r 30"));
		QVERIFY(joined.contains("-pix_fmt yuv420p"));
		QVERIFY(joined.contains("-s 1280x720"));
		QVERIFY(joined.contains("-f flv"));
		QVERIFY(joined.contains("-progress pipe:1"));
		QVERIFY(joined.contains("rtmp://rtmp.example.com/livehime"));
		QVERIFY(!joined.contains("gdigrab"));
	}

	void autoEncoderAndRateOmitArguments()
	{
		StreamSettings s;
		s.source = CaptureSource::Camera;
		s.server = "rtmp://rtmp.example.com";
		s.cameraName = "cam";
		s.microphoneName = "mic";
		s.encoder = "Auto";
		s.bitrate = "Auto";
		s.fps = "Auto";

		const QString joined = buildStreamArgs(s).join(' ');
		QVERIFY(!joined.contains("-c:v"));
		QVERIFY(!joined.contains("-b:v"));
		QVERIFY(!joined.contains("-r "));
	}

	void desktopSourceUsesGdigrab()
	{
		StreamSettings s;
		s.source = CaptureSource::Desktop;
		s.server = "rtmp://rtmp.example.com";
		s.microphoneName = "mic";
		s.fps = "Auto";

		const QStringList args = buildStreamArgs(s);
		const QString joined = args.join(' ');
		QVERIFY(joined.contains("-f gdigrab"));
		QVERIFY(joined.contains("-framerate 30"));
		QVERIFY(joined.contains("-i desktop"));
		QVERIFY(joined.contains("-i audio=mic"));
	}

	void windowSourceTargetsTitleWithProbeSize()
	{
		StreamSettings s;
		s.source = CaptureSource::Window;
		s.server = "rtmp://rtmp.example.com";
		s.microphoneName = "mic";
		s.windowTitle = "Notepad - 文档.txt";

		const QString joined = buildStreamArgs(s).join(' ');
		QVERIFY(joined.contains("-probesize 50000000"));
		QVERIFY(joined.contains("-i title=Notepad - 文档.txt"));
		QVERIFY(!joined.contains("desktop"));
	}

	void localFilePushesInRealtimeWithoutMicrophone()
	{
		const QString path = QDir::temp().absoluteFilePath("postlive-test-input.mp4");
		QFile file(path);
		QVERIFY(file.open(QIODevice::WriteOnly));

		StreamSettings s;
		s.source = CaptureSource::LocalFile;
		s.server = "rtmp://rtmp.example.com";
		s.filePath = path;

		const QStringList args = buildStreamArgs(s);
		const QString joined = args.join(' ');
		QVERIFY(joined.contains("-re"));
		QVERIFY(joined.contains("-i " + path));
		QVERIFY(!joined.contains("dshow"));
		QVERIFY(!joined.contains("audio="));
	}

	void invalidSettingsAreRejected()
	{
		QString error;

		StreamSettings s;
		s.source = CaptureSource::Camera;
		s.server = "not a url";
		QVERIFY(buildStreamArgs(s, &error).isEmpty());
		QVERIFY(!error.isEmpty());

		s.server = "rtmp://rtmp.example.com";
		s.cameraName.clear();
		QVERIFY(buildStreamArgs(s, &error).isEmpty());
		QVERIFY(error == "no camera");

		s.source = CaptureSource::Window;
		s.cameraName = "cam";
		s.windowTitle.clear();
		QVERIFY(buildStreamArgs(s, &error).isEmpty());
		QVERIFY(error == "no window selected");

		s.windowTitle = "win";
		s.source = CaptureSource::LocalFile;
		s.filePath = "Z:/definitely/missing/file.mp4";
		QVERIFY(buildStreamArgs(s, &error).isEmpty());
		QVERIFY(error == "file missing");
	}

	void watermarkOnlyWhenFileExists()
	{
		const QString watermark = QDir::temp().absoluteFilePath("postlive-test-watermark.png");
		QFile file(watermark);
		QVERIFY(file.open(QIODevice::WriteOnly));

		StreamSettings s;
		s.source = CaptureSource::Camera;
		s.server = "rtmp://rtmp.example.com";
		s.cameraName = "cam";
		s.microphoneName = "mic";
		s.watermark = true;
		s.watermarkFile = watermark;

		QVERIFY(buildStreamArgs(s).join(' ').contains("overlay=5:5"));

		s.watermarkFile = "Z:/missing/logo.png";
		const QString joined = buildStreamArgs(s).join(' ');
		QVERIFY(!joined.contains("overlay=5:5"));
		QVERIFY(!joined.contains(watermark));
	}
};

QTEST_MAIN(TestStreamArgs)
#include "test_stream_args.moc"
