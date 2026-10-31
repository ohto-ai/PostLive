#include <QtTest>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QWidget>

#include "Logging.h"
#include "WidgetConfigInfo.h"
#include "utils.h"

using namespace thatboy;

// Headless unit tests for the core layer (utils, config, token storage).
// They never construct top-level windows and never open message boxes, so
// they run fine with QT_QPA_PLATFORM=offscreen.
class TestCore : public QObject
{
	Q_OBJECT

	QTemporaryDir tempDir;
	QString originalWorkingDir;

	static void removeLocalFile(const QString& name)
	{
		QFile::remove(QDir(QDir::currentPath()).absoluteFilePath(name));
	}

	static bool writeLocalFile(const QString& name, const QString& content)
	{
		QFile file(QDir(QDir::currentPath()).absoluteFilePath(name));
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
			return false;
		return file.write(content.toUtf8()) != -1 && file.flush();
	}

private slots:
	void initTestCase()
	{
		QVERIFY(tempDir.isValid());
		originalWorkingDir = QDir::currentPath();
		// config files (application.json / users.json) are addressed relative
		// to the working directory, so run every test inside a temp dir.
		QVERIFY(QDir::setCurrent(tempDir.path()));
	}

	void cleanup()
	{
		removeLocalFile(QStringLiteral("application.json"));
		removeLocalFile(QStringLiteral("users.json"));
		thatboy::storage::config = nlohmann::json::object();
	}

	void cleanupTestCase()
	{
		QDir::setCurrent(originalWorkingDir);
	}

	void jsonQStringRoundtrip()
	{
		nlohmann::json j;
		thatboy::utils::to_json(j, QStringLiteral("你好 hello \"quoted\""));
		QVERIFY(j.is_string());

		QString out;
		thatboy::utils::from_json(j, out);
		QCOMPARE(out, QStringLiteral("你好 hello \"quoted\""));
	}

	void jsonQStringListRoundtrip()
	{
		const QStringList list{ "a", "b-2", "中文" };
		nlohmann::json j;
		thatboy::utils::to_json(j, list);
		QVERIFY(j.is_array());

		QStringList out;
		thatboy::utils::from_json(j, out);
		QCOMPARE(out, list);
	}

	void qRectJsonRoundtrip()
	{
		const QRect rect(11, 22, 333, 444);
		nlohmann::json j;
		thatboy::to_json(j, rect);
		QVERIFY(j.contains("x") && j.contains("y")
			&& j.contains("width") && j.contains("height"));

		QRect back;
		thatboy::from_json(j, back);
		QCOMPARE(back, rect);
	}

	void widgetConfigInfoJsonRoundtrip()
	{
		QWidget source;
		source.setGeometry(11, 22, 333, 444);
		source.setWindowOpacity(0.5);
		source.setAttribute(Qt::WA_TranslucentBackground, true);

		nlohmann::json j;
		thatboy::to_json(j, thatboy::WidgetConfigInfo(source));

		thatboy::WidgetConfigInfo restored;
		thatboy::from_json(j, restored);

		QWidget target;
		restored.config(target);
		QCOMPARE(target.geometry(), source.geometry());
		// Window opacity is stored internally as an 8-bit alpha value.
		QVERIFY(qAbs(target.windowOpacity() - 0.5) < 0.01);
		QCOMPARE(target.testAttribute(Qt::WA_TranslucentBackground), true);
	}

	void md5KnownVector()
	{
		QCOMPARE(thatboy::utils::generateMD5(QStringLiteral("abc")),
			QStringLiteral("900150983CD24FB0D6963F7D28E17F72"));
		QCOMPARE(thatboy::utils::generateMD5(QString()),
			QStringLiteral("D41D8CD98F00B204E9800998ECF8427E"));
	}

	void loadConfigDefaultsWhenMissing()
	{
		QVERIFY(!QFile::exists(QStringLiteral("application.json")));
		thatboy::utils::loadConfig();

		const auto& config = thatboy::storage::config;
		QVERIFY(config.is_object());
		QCOMPARE(config["auth_server"].get<std::string>(), std::string("http://localhost:8877"));
		QCOMPARE(config["website"].get<std::string>(), std::string("https://ohtoai.top"));
		QCOMPARE(config["user_list"].get<std::string>(), std::string("users.json"));
		QCOMPARE(config["login"]["auto_login"].get<bool>(), false);
		QCOMPARE(config["stream"]["bitrate"].get<std::string>(), std::string("Auto"));
		QCOMPARE(config["stream"]["watermark"].get<bool>(), true);
		QVERIFY(config["widget_info"].contains("login"));
		QVERIFY(config["widget_info"]["login"]["geometry"].is_object());
		QCOMPARE(thatboy::utils::authServerUrl(), QStringLiteral("http://localhost:8877"));
	}

	void loadConfigUsesDefaultsWhenInvalid()
	{
		QVERIFY(writeLocalFile(QStringLiteral("application.json"),
			"{ this is not valid json"));
		thatboy::utils::loadConfig();

		QCOMPARE(thatboy::storage::config["auth_server"].get<std::string>(),
			std::string("http://localhost:8877"));
		QVERIFY(thatboy::storage::config["stream"].is_object());
	}

	void loadConfigMergesUserOverrides()
	{
		QVERIFY(writeLocalFile(QStringLiteral("application.json"),
			QStringLiteral("{\"auth_server\": \"https://auth.example.com\", \"stream\": {\"bitrate\": \"2500\"}}")));
		thatboy::utils::loadConfig();

		const auto& config = thatboy::storage::config;
		QCOMPARE(config["auth_server"].get<std::string>(), std::string("https://auth.example.com"));
		QCOMPARE(config["stream"]["bitrate"].get<std::string>(), std::string("2500"));
		// Untouched values still come from the defaults.
		QCOMPARE(config["stream"]["fps"].get<std::string>(), std::string("Auto"));
		QCOMPARE(thatboy::utils::authServerUrl(), QStringLiteral("https://auth.example.com"));
	}

	void loadConfigFillsMissingSections()
	{
		QVERIFY(writeLocalFile(QStringLiteral("application.json"),
			QStringLiteral("{\"auth_server\": \"http://127.0.0.1:9000\"}")));
		thatboy::utils::loadConfig();

		const auto& config = thatboy::storage::config;
		QVERIFY(config["login"].is_object());
		QVERIFY(config["login"].contains("auto_login"));
		QVERIFY(config["stream"].is_object());
		QVERIFY(config["widget_info"].is_object());
		for (const char* name : { "login", "login_setting", "platform" })
		{
			QVERIFY(config["widget_info"].contains(name));
			QVERIFY(config["widget_info"][name]["geometry"].is_object());
			QVERIFY(config["widget_info"][name].contains("opacity"));
			QVERIFY(config["widget_info"][name].contains("translucent"));
		}
	}

	void saveConfigRoundtrip()
	{
		thatboy::utils::loadConfig();
		thatboy::storage::config["auth_server"] = "http://127.0.0.1:8877";
		QVERIFY(thatboy::utils::saveConfig());

		QFile file(QStringLiteral("application.json"));
		QVERIFY(file.open(QIODevice::ReadOnly));
		const auto saved = nlohmann::json::parse(
			std::string(file.readAll().constData(), size_t(file.size())), nullptr, false);
		QVERIFY(!saved.is_discarded());
		QCOMPARE(saved, thatboy::storage::config);
	}

	void loadUserDataMissingFileFallsBackToDefaults()
	{
		QVERIFY(!QFile::exists(QStringLiteral("users.json")));
		thatboy::utils::loadConfig();
		thatboy::utils::loadUserData();

		const auto& users = thatboy::storage::usersStorage;
		QVERIFY(users.is_object());
		QVERIFY(users["user_names"].is_array());
		QVERIFY(users["users"].is_object());
		QVERIFY(users["users"].empty());
	}

	void tokenDpapiRoundtrip()
	{
		const QString token = QStringLiteral("s3cret-token-秘密123");
		const QString stored = thatboy::utils::protectToken(token);
		QVERIFY(stored.startsWith(QStringLiteral("dpapi:")));
		QVERIFY(stored != token);
		QCOMPARE(thatboy::utils::unprotectToken(stored), token);
	}

	void tokenDpapiIdempotent()
	{
		const QString stored = thatboy::utils::protectToken(QStringLiteral("abc"));
		QCOMPARE(thatboy::utils::protectToken(stored), stored);
	}

	void tokenEmptyAndPlaintextPassThrough()
	{
		QCOMPARE(thatboy::utils::protectToken(QString()), QString());
		QCOMPARE(thatboy::utils::unprotectToken(QString()), QString());
		// Reading a legacy plaintext value returns it unchanged...
		QCOMPARE(thatboy::utils::unprotectToken(QStringLiteral("legacy-plain")),
			QStringLiteral("legacy-plain"));
		// ...while saving encrypts everything that is not already protected.
		const QString stored = thatboy::utils::protectToken(QStringLiteral("legacy-plain"));
		QVERIFY(stored.startsWith(QStringLiteral("dpapi:")));
		QCOMPARE(thatboy::utils::unprotectToken(stored), QStringLiteral("legacy-plain"));
	}
};

QTEST_MAIN(TestCore)
#include "test_core.moc"
