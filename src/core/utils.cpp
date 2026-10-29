#include "utils.h"

#include <QDebug>
#include <QFile>
#include <QUrl>
#include <string>

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
	const std::string userListPath = storage::config.value("user_list", std::string("users.json"));
	bool ok = false;
	auto users = readJsonFile(QString::fromStdString(userListPath), &ok);
	if (!ok)
	{
		qWarning() << "users.json is missing or invalid; using built-in defaults";
		users = readJsonFile(QStringLiteral(":/LiveLoginDialog/config/users.default.json"));
	}
	if (!users.is_object())
		users = nlohmann::json::object();
	if (!users.contains("user_names") || !users["user_names"].is_array())
		users["user_names"] = nlohmann::json::array();
	if (!users.contains("users") || !users["users"].is_object())
		users["users"] = nlohmann::json::object();
	storage::usersStorage = users;

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
		avatar = QPixmap{ user["avatar_file"] };
		if (avatar.isNull())
		{
			user["avatar_file"] = QString::asprintf("user/avatar/%s.avatar", userName.c_str());
			storage::avatarDownloader.download(user["avatar"], user["avatar_file"]);
		}
	}
}

bool thatboy::utils::saveUserData()
{
	const std::string userListPath = storage::config.value("user_list", std::string("users.json"));
	std::ofstream ofs(userListPath);
	if (!ofs)
	{
		qWarning() << "failed to save user data to" << QString::fromStdString(userListPath);
		return false;
	}
	ofs << storage::usersStorage.dump(4);
	return ofs.good();
}

QString thatboy::utils::generateMD5(QString str)
{
	return QCryptographicHash::hash(str.toUtf8()
		, QCryptographicHash::Md5).toHex().toUpper();
}

std::unique_ptr<httplib::Client> thatboy::utils::createAuthClient()
{
	const std::string raw = storage::config.value("auth_server", std::string("http://localhost:8877"));
	const QUrl url(QString::fromStdString(raw));
	if (!url.isValid() || url.host().isEmpty())
		return nullptr;

	const QString scheme = url.scheme().toLower();
	if (scheme != "http")
		return nullptr;

	const int port = url.port(80);
	const QString base = scheme + "://" + url.host() + ":" + QString::number(port);
	auto client = std::make_unique<httplib::Client>(base.toStdString());
	client->set_connection_timeout(3, 0);
	client->set_read_timeout(5, 0);
	return client;
}

void thatboy::utils::loadConfig()
{
	const auto defaults = readJsonFile(QStringLiteral(":/LiveLoginDialog/config/application.default.json"));
	bool ok = false;
	const auto userConfig = readJsonFile(QString::fromUtf8(storage::ConfigFilePath), &ok);

	if (ok)
	{
		storage::config = defaults.is_object() ? defaults : nlohmann::json::object();
		storage::config.merge_patch(userConfig);
	}
	else
	{
		qWarning() << "application.json is missing or invalid; using built-in defaults";
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
	if (!storage::config.contains("user_list") || !storage::config["user_list"].is_string())
		storage::config["user_list"] = "users.json";
}

bool thatboy::utils::saveConfig()
{
	std::ofstream ofs(storage::ConfigFilePath);
	if (!ofs)
	{
		qWarning() << "failed to save application config to" << storage::ConfigFilePath;
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