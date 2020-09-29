#include "utils.h"

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
	std::ifstream ifs(storage::config["user_list"].get<std::string>());
	storage::usersStorage = nlohmann::json::parse(std::string{ (std::istreambuf_iterator<char>(ifs)),
		(std::istreambuf_iterator<char>()) });

	// load avatars
	QObject::connect(&storage::avatarDownloader, &AsyncDownloader::finished, [&](QString url, QString file, bool success)
		{
			if (success)
				storage::userAvatarCache[QFileInfo(file).completeBaseName().toStdString()] = QPixmap{ file };
			emit storage::generalSignal.avatarDownloaded(QFileInfo(file).completeBaseName(), success);
		});

	for (std::string userName : storage::usersStorage["user_names"])
	{
		auto& avatar = storage::userAvatarCache[userName];
		auto& user = storage::usersStorage["users"][userName];

		if (!user.contains("avatar_file"))
			user["avatar_file"] = "";
		avatar = QPixmap{ user["avatar_file"] };
		if (avatar.isNull())
		{
			user["avatar_file"] = QString::asprintf("user/avatar/%s.avatar", userName.c_str());
			storage::avatarDownloader.download(user["avatar"], user["avatar_file"]);
		}
	}
}

void thatboy::utils::saveUserData()
{
	std::ofstream ofs(storage::config["user_list"].get<std::string>());
	ofs << storage::usersStorage.dump(4);
}

QString thatboy::utils::generateMD5(QString str)
{
	return QCryptographicHash::hash(str.toUtf8()
		, QCryptographicHash::Md5).toHex().toUpper();
}

void thatboy::utils::loadConfig()
{
	std::ifstream ifs(storage::ConfigFilePath);
	storage::config = nlohmann::json::parse(std::string{ (std::istreambuf_iterator<char>(ifs)),
		(std::istreambuf_iterator<char>()) });
}

void thatboy::utils::saveConfig()
{
	std::ofstream ofs(storage::ConfigFilePath);
	ofs << storage::config.dump(4);
}

nlohmann::json thatboy::storage::usersStorage;
nlohmann::json thatboy::storage::config;
nlohmann::json thatboy::storage::currentUser;
httplib::Client thatboy::storage::accountVerifyClient{ "localhost",8080 };
std::map<std::string, QPixmap> thatboy::storage::userAvatarCache;
AsyncDownloader thatboy::storage::avatarDownloader;
GeneralSignal thatboy::storage::generalSignal;