#include <QtTest>

#include "StreamArgs.h"
#include "StreamSource.h"

// Contract tests for the scene-based ffmpeg.exe argument builder.
class TestSceneArgs : public QObject
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

	static StreamSource makeSource(StreamSource::Kind kind, const QString& name)
	{
		StreamSource source;
		source.kind = kind;
		source.name = name;
		return source;
	}

private slots:
	void jsonRoundtrip()
	{
		const StreamSource source = makeSource(StreamSource::Kind::Window, "Notepad");
		nlohmann::json j = source;
		const StreamSource back = j.get<StreamSource>();
		QVERIFY(source == back);

		// Malformed kind falls back to Camera instead of invoking UB.
		const StreamSource clamped = nlohmann::json({ { "kind", 99 }, { "name", "x" } }).get<StreamSource>();
		QCOMPARE(clamped.kind, StreamSource::Kind::Camera);
	}

	void singleSourceKeepsLegacyPipeline()
	{
		const QList<StreamSource> scene{ makeSource(StreamSource::Kind::Camera, "USB Camera") };
		const QStringList args = buildSceneArgs(scene, baseSettings());
		const QString joined = args.join(' ');

		QVERIFY(!args.isEmpty());
		QVERIFY(joined.contains("-i video=USB Camera"));
		QVERIFY(joined.contains("-i audio=virtual-audio-capturer"));
		QVERIFY(!joined.contains("filter_complex"));
		QVERIFY(!joined.contains("-map [vout]"));
		QVERIFY(joined.contains("-map 1:a"));
		QVERIFY(joined.contains("-f flv"));
		QVERIFY(joined.contains("-progress pipe:1"));
		QVERIFY(joined.contains("rtmp://rtmp.example.com/livehime"));
		QVERIFY(joined.contains("-s 1280x720"));
	}

	void twoSourcesCompositeWithOverlay()
	{
		const QList<StreamSource> scene{
			makeSource(StreamSource::Kind::Desktop, "desktop"),
			makeSource(StreamSource::Kind::Camera, "USB Camera"),
		};
		const QStringList args = buildSceneArgs(scene, baseSettings());
		const QString joined = args.join(' ');

		// Inputs: 0 gdigrab, 1 camera, 2 microphone.
		QVERIFY(joined.contains("-f gdigrab"));
		QVERIFY(joined.contains("-i desktop"));
		QVERIFY(joined.contains("-i video=USB Camera"));
		QVERIFY(joined.contains("-i audio=virtual-audio-capturer"));
		QVERIFY(joined.contains("-map 2:a"));
		QVERIFY(joined.contains("[1:v]scale=480:-1[s1];[0:v][s1]overlay=x=main_w-w-24:y=main_h-h-24[vout]"));
		QVERIFY(joined.contains("-map [vout]"));
	}

	void windowSourceAddsProbeSize()
	{
		const QList<StreamSource> scene{ makeSource(StreamSource::Kind::Window, "Doc.txt") };
		const QString joined = buildSceneArgs(scene, baseSettings()).join(' ');
		QVERIFY(joined.contains("-probesize 50000000"));
		QVERIFY(joined.contains("-i title=Doc.txt"));
	}

	void fileSourcePushesRealtimeAndKeepsOwnAudio()
	{
		const QString path = QDir::temp().absoluteFilePath("postlive-scene-test.mp4");
		QFile file(path);
		QVERIFY(file.open(QIODevice::WriteOnly));

		const QList<StreamSource> scene{ makeSource(StreamSource::Kind::LocalFile, path) };
		StreamSettings settings = baseSettings();
		settings.microphoneName.clear(); // file carries its own audio
		const QString joined = buildSceneArgs(scene, settings).join(' ');

		QVERIFY(joined.contains("-re"));
		QVERIFY(joined.contains("-i " + path));
		QVERIFY(joined.contains("-map 0:a?"));
		QVERIFY(!joined.contains("dshow"));
	}

	void watermarkBecomesTopLayer()
	{
		const QString watermark = QDir::temp().absoluteFilePath("postlive-scene-logo.png");
		QFile file(watermark);
		QVERIFY(file.open(QIODevice::WriteOnly));

		const QList<StreamSource> scene{
			makeSource(StreamSource::Kind::Desktop, "desktop"),
			makeSource(StreamSource::Kind::Camera, "cam"),
		};
		StreamSettings settings = baseSettings();
		settings.watermark = true;
		settings.watermarkFile = watermark;

		const QString joined = buildSceneArgs(scene, settings).join(' ');
		// Inputs: 0 gdigrab, 1 camera, 2 watermark, 3 microphone.
		QVERIFY(joined.contains("-i " + watermark));
		QVERIFY(joined.contains("[2:v]scale=120:-1[s2];[o1][s2]overlay=x=5:y=5[vout]"));
		QVERIFY(joined.contains("-map 3:a"));
	}

	void invalidScenesAreRejected()
	{
		QString error;
		StreamSettings settings = baseSettings();

		QVERIFY(buildSceneArgs({}, settings, &error).isEmpty());
		QVERIFY(error == "no source");

		const QList<StreamSource> noCamera{ makeSource(StreamSource::Kind::Camera, QString()) };
		QVERIFY(buildSceneArgs(noCamera, settings, &error).isEmpty());
		QVERIFY(error == "no camera");

		const QList<StreamSource> noWindow{ makeSource(StreamSource::Kind::Window, QString()) };
		QVERIFY(buildSceneArgs(noWindow, settings, &error).isEmpty());
		QVERIFY(error == "no window selected");

		const QList<StreamSource> missingFile{ makeSource(StreamSource::Kind::LocalFile, "Z:/missing.mp4") };
		QVERIFY(buildSceneArgs(missingFile, settings, &error).isEmpty());
		QVERIFY(error == "file missing");

		settings.server = "not a url";
		const QList<StreamSource> ok{ makeSource(StreamSource::Kind::Desktop, "desktop") };
		QVERIFY(buildSceneArgs(ok, settings, &error).isEmpty());
		QVERIFY(error == "invalid server url");
	}

	void optionalFpsOnlyForCameraBase()
	{
		StreamSettings settings = baseSettings();
		settings.fps = "60";

		const QString camera = buildSceneArgs(
			{ makeSource(StreamSource::Kind::Camera, "cam") }, settings).join(' ');
		QVERIFY(camera.contains("-r 60"));

		const QString desktop = buildSceneArgs(
			{ makeSource(StreamSource::Kind::Desktop, "desktop") }, settings).join(' ');
		QVERIFY(!desktop.contains("-r 60"));
		QVERIFY(desktop.contains("-framerate 60"));
	}
};

QTEST_MAIN(TestSceneArgs)
#include "test_scene_args.moc"
