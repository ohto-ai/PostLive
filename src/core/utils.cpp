#include "utils.h"

#include "Logging.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QStringList>
#include <QUrl>
#include <string>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <dpapi.h>
#endif

namespace
{
	nlohmann::json readJsonFile(const QString& path, bool* ok = nullptr)
	{
		if (ok)
			*ok = false;

		QFile file(path);
		if (!file.open(QIODevice::ReadOnly))
			return nlohmann::json();

		const QByteArray data = file.readAll();
		const auto json = nlohmann::json::parse(
			std::string(data.constData(), static_cast<size_t>(data.size())), nullptr, false);
		if (ok)
			*ok = !json.is_discarded() && json.is_object();
		return json;
	}
}

QString thatboy::storage::dataDir;

QString thatboy::storage::resolveDataPath(const QString& relative)
{
	return dataDir.isEmpty() ? relative : dataDir + QStringLiteral("/") + relative;
}

// One-time migration from the exe-relative layout: config, user data and the
// avatar cache move into the writable data directory (AppData) so installed
// builds work without write access to Program Files.
void thatboy::storage::migrateLegacyFiles()
{
	const QString exeDir = QCoreApplication::applicationDirPath();
	for (const QString& name : { QStringLiteral("application.json"), QStringLiteral("users.json") })
	{
		const QString legacy = exeDir + QStringLiteral("/") + name;
		const QString target = resolveDataPath(name);
		if (QFileInfo::exists(legacy) && !QFileInfo::exists(target))
		{
			QDir().mkpath(dataDir);
			if (!QFile::rename(legacy, target))
				QFile::copy(legacy, target);
		}
	}

	const QString legacyAvatars = exeDir + QStringLiteral("/user/avatar");
	const QString targetAvatars = resolveDataPath(QStringLiteral("user/avatar"));
	QDir sourceDir(legacyAvatars);
	if (sourceDir.exists() && !QDir(targetAvatars).exists())
	{
		QDir().mkpath(targetAvatars);
		for (const QFileInfo& entry :
			sourceDir.entryInfoList(QStringList{ QStringLiteral("*.avatar") }, QDir::Files))
			QFile::rename(entry.absoluteFilePath(),
				targetAvatars + QStringLiteral("/") + entry.fileName());
	}
}

QPixmap thatboy::utils::roundedPixmap(const QPixmap& src)
{
	if (src.isNull()) {
		return QPixmap();
	}
	int radius = std::max(src.width(), src.height());
	QSize size(2 * radius, 2 * radius);
	QBitmap mask(size);
	QPainter painter(&mask);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);
	painter.fillRect(0, 0, size.width(), size.height(), Qt::white);
	painter.setBrush(QColor(0, 0, 0));
	painter.drawRoundedRect(0, 0, size.width(), size.height(), radius * 2 - 1, radius * 2 - 1);
	QPixmap image = src.scaled(size);
	image.setMask(mask);
	return image;
}

void thatboy::utils::from_json(const nlohmann::json& j, QString& string)
{
	string = QString::fromStdString(j);
}

void thatboy::utils::from_json(const nlohmann::json& j, QStringList& stringList)
{
	stringList.clear();
	for (auto& str : j)
		stringList.push_back(str);
}

void thatboy::utils::to_json(nlohmann::json& j, const QString& string)
{
	j = string.toStdString();
}

void thatboy::utils::to_json(nlohmann::json& j, const QStringList& stringList)
{
	j.clear();
	for (auto& str : stringList)
		j.push_back(str.toStdString());
}

void thatboy::utils::loadUserData()
{
	const std::string userListName = storage::config.value("user_list", std::string("users.json"));
	const QString userListPath = storage::resolveDataPath(QString::fromStdString(userListName));
	bool ok = false;
	auto users = readJsonFile(userListPath, &ok);
	if (!ok)
	{
		qCWarning(lcCore) << "users.json is missing or invalid; using built-in defaults";
		users = readJsonFile(QStringLiteral(":/LiveLoginDialog/config/users.default.json"));
	}
	if (!users.is_object())
		users = nlohmann::json::object();
	if (!users.contains("user_names") || !users["user_names"].is_array())
		users["user_names"] = nlohmann::json::array();
	if (!users.contains("users") || !users["users"].is_object())
		users["users"] = nlohmann::json::object();
	storage::usersStorage = users;

	// Decrypt tokens at rest. Plaintext values from older versions are kept as-is.
	if (storage::usersStorage["users"].is_object())
	{
		for (auto& item : storage::usersStorage["users"].items())
		{
			auto& user = item.value();
			if (!user.is_object() || !user.contains("token") || !user["token"].is_string())
				continue;

			const QString stored = QString::fromStdString(user["token"].get<std::string>());
			const QString token = unprotectToken(stored);
			if (stored.startsWith(QStringLiteral("dpapi:")) && token.isEmpty())
			{
				user["token"] = "";
				user["remember_password"] = false;
			}
			else
			{
				user["token"] = token.toStdString();
			}
		}
	}

	// load avatars
	QObject::connect(&storage::avatarDownloader, &AsyncDownloader::finished, [&](QString url, QString file, bool success)
		{
			if (success)
				storage::userAvatarCache[QFileInfo(file).completeBaseName().toStdString()] = QPixmap{ file };
			emit storage::generalSignal.avatarDownloaded(QFileInfo(file).completeBaseName(), success);
		});

	for (const auto& userNameJson : storage::usersStorage["user_names"])
	{
		if (!userNameJson.is_string())
			continue;
		const std::string userName = userNameJson.get<std::string>();

		if (!storage::usersStorage["users"].contains(userName)
			|| !storage::usersStorage["users"][userName].is_object())
			continue;

		auto& user = storage::usersStorage["users"][userName];
		if (!user.contains("avatar") || !user["avatar"].is_string())
			continue;

		if (!user.contains("avatar_file") || !user["avatar_file"].is_string())
			user["avatar_file"] = "";

		auto& avatar = storage::userAvatarCache[userName];
		avatar = QPixmap{ storage::resolveDataPath(
			QString::fromStdString(user["avatar_file"].get<std::string>())) };
		if (avatar.isNull())
		{
			user["avatar_file"] = QString::asprintf("user/avatar/%s.avatar", userName.c_str());
			storage::avatarDownloader.download(user["avatar"],
				storage::resolveDataPath(QString::fromStdString(
					user["avatar_file"].get<std::string>())));
		}
	}
}

bool thatboy::utils::saveUserData()
{
	const std::string userListName = storage::config.value("user_list", std::string("users.json"));
	const QString userListPath = storage::resolveDataPath(QString::fromStdString(userListName));
	std::ofstream ofs(userListPath.toStdString());
	if (!ofs)
	{
		qCWarning(lcCore) << "failed to save user data to" << userListPath;
		return false;
	}

	auto serialized = storage::usersStorage;
	if (serialized.contains("users") && serialized["users"].is_object())
	{
		for (auto& item : serialized["users"].items())
		{
			auto& user = item.value();
			if (!user.is_object() || !user.contains("token") || !user["token"].is_string())
				continue;

			const QString token = QString::fromStdString(user["token"].get<std::string>());
			user["token"] = protectToken(token).toStdString();
		}
	}

	ofs << serialized.dump(4);
	return ofs.good();
}

QString thatboy::utils::generateMD5(QString str)
{
	return QCryptographicHash::hash(str.toUtf8()
		, QCryptographicHash::Md5).toHex().toUpper();
}

QString thatboy::utils::authServerUrl()
{
	return QString::fromStdString(
		storage::config.value("auth_server", std::string("http://localhost:8877")));
}

QString thatboy::utils::protectToken(const QString& token)
{
#ifdef _WIN32
	if (token.isEmpty() || token.startsWith(QStringLiteral("dpapi:")))
		return token;

	const QByteArray utf8 = token.toUtf8();
	DATA_BLOB input{};
	input.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(utf8.constData()));
	input.cbData = static_cast<DWORD>(utf8.size());

	DATA_BLOB output{};
	if (!CryptProtectData(&input, L"PostLive token", nullptr, nullptr, nullptr,
		CRYPTPROTECT_UI_FORBIDDEN, &output))
	{
		qCWarning(lcCore) << "CryptProtectData failed for token";
		return token;
	}

	const QByteArray encrypted(reinterpret_cast<const char*>(output.pbData),
		static_cast<int>(output.cbData));
	LocalFree(output.pbData);
	return QStringLiteral("dpapi:") + QString::fromLatin1(encrypted.toBase64());
#else
	return token;
#endif
}

QString thatboy::utils::unprotectToken(const QString& stored)
{
#ifdef _WIN32
	if (!stored.startsWith(QStringLiteral("dpapi:")))
		return stored;

	const QByteArray encrypted = QByteArray::fromBase64(stored.mid(6).toLatin1());
	if (encrypted.isEmpty())
		return {};

	DATA_BLOB input{};
	input.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(encrypted.constData()));
	input.cbData = static_cast<DWORD>(encrypted.size());

	DATA_BLOB output{};
	if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr,
		CRYPTPROTECT_UI_FORBIDDEN, &output))
	{
		qCWarning(lcCore) << "CryptUnprotectData failed for token";
		return {};
	}

	const QString token = QString::fromUtf8(reinterpret_cast<const char*>(output.pbData),
		static_cast<int>(output.cbData));
	LocalFree(output.pbData);
	return token;
#else
	return stored;
#endif
}

void thatboy::utils::loadConfig()
{
	const auto defaults = readJsonFile(QStringLiteral(":/LiveLoginDialog/config/application.default.json"));
	bool ok = false;
	const auto userConfig = readJsonFile(storage::resolveDataPath(QStringLiteral("application.json")), &ok);

	if (ok)
	{
		storage::config = defaults.is_object() ? defaults : nlohmann::json::object();
		storage::config.merge_patch(userConfig);
	}
	else
	{
		qCWarning(lcCore) << "application.json is missing or invalid; using built-in defaults";
		storage::config = defaults.is_object() ? defaults : nlohmann::json::object();
	}

	if (!storage::config.contains("login") || !storage::config["login"].is_object())
	{
		storage::config["login"] = {
			{ "auto_login", false },
			{ "current_user", "" }
		};
	}
	if (!storage::config.contains("stream") || !storage::config["stream"].is_object())
	{
		storage::config["stream"] = {
			{ "encoder", "Auto" },
			{ "bitrate", "Auto" },
			{ "fps", "Auto" },
			{ "preview", true }
		};
	}
	if (!storage::config["stream"].contains("watermark")
		|| !storage::config["stream"]["watermark"].is_boolean())
	{
		storage::config["stream"]["watermark"] = true;
	}
	if (!storage::config.contains("auth_server") || !storage::config["auth_server"].is_string())
		storage::config["auth_server"] = "http://localhost:8877";
	if (!storage::config.contains("website") || !storage::config["website"].is_string())
		storage::config["website"] = "https://ohtoai.top";
	storage::websiteUrl = QString::fromStdString(storage::config["website"].get<std::string>());
	if (!storage::config.contains("widget_info") || !storage::config["widget_info"].is_object())
		storage::config["widget_info"] = nlohmann::json::object();
	for (const char* name : { "login", "login_setting", "platform" })
	{
		if (!storage::config["widget_info"].contains(name)
			|| !storage::config["widget_info"][name].is_object())
		{
			storage::config["widget_info"][name] = {
				{ "geometry", { { "x", 100 }, { "y", 100 }, { "width", 600 }, { "height", 450 } } },
				{ "opacity", 1.0 },
				{ "translucent", true }
			};
		}
	}
	// The platform window grew to fit the scene editor; stale small geometry
	// from older builds is bumped so the preview stays readable.
	{
		auto& platformInfo = storage::config["widget_info"]["platform"];
		if (platformInfo["geometry"]["width"].is_number()
			&& platformInfo["geometry"]["width"].get<int>() < 1000)
		{
			platformInfo = {
				{ "geometry", { { "x", 60 }, { "y", 60 }, { "width", 1280 }, { "height", 800 } } },
				{ "opacity", 1.0 },
				{ "translucent", false }
			};
		}
	}
	// The login dialog was redesigned at a fixed 760x480; stale geometry from
	// the 600x450 skin (and its translucency) would fight the new layout.
	auto& loginInfo = storage::config["widget_info"]["login"];
	if (!loginInfo["geometry"].is_object()
		|| !loginInfo["geometry"]["width"].is_number()
		|| loginInfo["geometry"]["width"].get<int>() < 760)
	{
		loginInfo = {
			{ "geometry", { { "x", 100 }, { "y", 100 }, { "width", 760 }, { "height", 480 } } },
			{ "opacity", 1.0 },
			{ "translucent", false }
		};
	}
	if (!storage::config.contains("user_list") || !storage::config["user_list"].is_string())
		storage::config["user_list"] = "users.json";
}

bool thatboy::utils::saveConfig()
{
	const QString configPath = storage::resolveDataPath(QStringLiteral("application.json"));
	std::ofstream ofs(configPath.toStdString());
	if (!ofs)
	{
		qCWarning(lcCore) << "failed to save application config to" << configPath;
		return false;
	}
	ofs << storage::config.dump(4);
	return ofs.good();
}

nlohmann::json thatboy::storage::usersStorage;
nlohmann::json thatboy::storage::config;
nlohmann::json thatboy::storage::currentUser;
QString thatboy::storage::websiteUrl;
std::map<std::string, QPixmap> thatboy::storage::userAvatarCache;
AsyncDownloader thatboy::storage::avatarDownloader;
GeneralSignal thatboy::storage::generalSignal;