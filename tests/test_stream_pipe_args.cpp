#include <QtTest>

#include "StreamArgs.h"

// Contract tests for the composited-canvas streaming argument builder.
class TestStreamPipeArgs : public QObject
{
	Q_OBJECT

	static StreamSettings baseSettings()
	{
		StreamSettings s;
		s.server = "rtmp://rtmp.example.com";
		s.microphoneName = "virtual-audio-capturer";
		s.size = "1280x720";
		return s;
	}

private slots:
	void pipeInputWithMicrophone()
	{
		const QStringList args = buildStreamPipeArgs(QSize(1280, 720), baseSettings(), true);
		const QString joined = args.join(' ');

		QVERIFY(!args.isEmpty());
		QVERIFY(joined.contains("-f rawvideo"));
		QVERIFY(joined.contains("-pix_fmt bgra"));
		QVERIFY(joined.contains("-s 1280x720"));
		QVERIFY(joined.contains("-r 30"));
		QVERIFY(joined.contains("-i -"));
		QVERIFY(joined.contains("-i audio=virtual-audio-capturer"));
		QVERIFY(joined.contains("-f flv"));
		QVERIFY(joined.contains("-progress pipe:1"));
		QVERIFY(joined.contains("rtmp://rtmp.example.com/livehime"));
	}

	void autoFpsBecomesThirty()
	{
		StreamSettings settings = baseSettings();
		settings.fps = "60";
		QVERIFY(buildStreamPipeArgs(QSize(1280, 720), settings, false).join(' ')
			.contains("-r 60"));
	}

	void audioDisabledOmitsDshow()
	{
		const QString joined = buildStreamPipeArgs(QSize(1280, 720), baseSettings(), false)
			.join(' ');
		QVERIFY(!joined.contains("dshow"));
		QVERIFY(!joined.contains("audio="));
	}

	void encoderAndBitratePassthrough()
	{
		StreamSettings settings = baseSettings();
		settings.encoder = "libx264";
		settings.bitrate = "2500";
		const QString joined = buildStreamPipeArgs(QSize(1280, 720), settings, false).join(' ');
		QVERIFY(joined.contains("-c:v libx264"));
		QVERIFY(joined.contains("-b:v 2500k"));
		QVERIFY(joined.contains("-pix_fmt yuv420p"));
	}

	void autoEncoderOmitsCodec()
	{
		const QString joined = buildStreamPipeArgs(QSize(1280, 720), baseSettings(), false).join(' ');
		QVERIFY(!joined.contains("-c:v"));
		QVERIFY(!joined.contains("-b:v"));
	}

	void invalidInputsAreRejected()
	{
		QString error;
		QVERIFY(buildStreamPipeArgs(QSize(), baseSettings(), false, &error).isEmpty());
		QVERIFY(error == "canvas size not ready");

		StreamSettings settings = baseSettings();
		settings.server = "not a url";
		QVERIFY(buildStreamPipeArgs(QSize(1280, 720), settings, false, &error).isEmpty());
		QVERIFY(error == "invalid server url");

		settings.server = "rtmp://rtmp.example.com";
		settings.microphoneName.clear();
		QVERIFY(buildStreamPipeArgs(QSize(1280, 720), settings, true, &error).isEmpty());
		QVERIFY(error == "no microphone");
	}
};

QTEST_MAIN(TestStreamPipeArgs)
#include "test_stream_pipe_args.moc"
