#pragma once

#include <QFileInfo>
#include <QPixmap>
#include <QImage>
#include <QBitmap>
#include <QPainter>
#include <QStringList>
#include <QCryptographicHash>
#include <QDateTime>
#include <QTimer>
#include <json.hpp>
#include <fstream>
#include <httplib.h>
#include "GeneralSignal.h"
#include "AsyncDownloader.h"
#include "WidgetConfigInfo.h"

namespace thatboy
{
	namespace utils
	{
		QPixmap roundedPixmap(const QPixmap& src);

		void from_json(const nlohmann::json& j, QString& string);
		void from_json(const nlohmann::json& j, QStringList& stringList);
		void to_json(nlohmann::json& j, const QString& string);
		void to_json(nlohmann::json& j, const QStringList& stringList);

		void loadConfig();
		void saveConfig();
		void loadUserData();
		void saveUserData();

		QString generateMD5(QString);
	}
	namespace storage
	{
		constexpr auto DefaultUserAvatarUrl{ ":/LiveLoginDialog/res/avatar.png" };
		constexpr auto DefaultMaleAvatarUrl{ ":/LiveLoginDialog/res/avatar-male.png" };
		constexpr auto DefaultFemaleAvatarUrl{ ":/LiveLoginDialog/res/avatar-female.png" };
		constexpr const char* DefaultAvatarUrlGroup[3]
		{
			DefaultUserAvatarUrl
			, DefaultMaleAvatarUrl
			, DefaultFemaleAvatarUrl
		};

		constexpr auto RegexAccount{ R"(^[A-Za-z][A-Za-z0-9_-]{3,15}$)" };
		constexpr auto RegexPassword{ R"(^[A-Za-z0-9._~!@#$^&*]{6,24}$)" };
		constexpr auto ConfigFilePath{ "application.json" };
		constexpr int LoginErrorLabelTime{ 8000 };

		constexpr auto RegisterUrl{ "http://live.thatboy.info/live/login.html" };
		constexpr auto ResetPasswordUrl{ "http://live.thatboy.info/live" };
		constexpr auto WebsiteUrl{ "http://thatboy.info" };
		constexpr auto GithubUrl{ "https://github.com/NOPornLivePlatform/LivePlatform" };

		extern httplib::Client accountVerifyClient;
		extern std::map<std::string, QPixmap> userAvatarCache;
		extern nlohmann::json config;
		extern nlohmann::json usersStorage;
		extern AsyncDownloader avatarDownloader;

		extern nlohmann::json currentUser;

		extern GeneralSignal generalSignal;
	}
}
using thatboy::utils::to_json;
using thatboy::utils::from_json;