#pragma once
#include <QString>
#include <QCryptographicHash>
#include <QDateTime>

namespace thatboy
{
	struct TimeStampToken
	{
		QString token;
		time_t timeStamp;
		TimeStampToken() = default;
		TimeStampToken(QString password)
		{
			generate(password);
		}
		TimeStampToken(const TimeStampToken&) = default;

		TimeStampToken& operator = (const TimeStampToken&) = default;
		void generate(QString password)
		{
			QString pwdMD5{ QCryptographicHash::hash(password.toUtf8()
			   , QCryptographicHash::Md5).toHex().toUpper() };
			timeStamp = QDateTime::currentDateTime().toTime_t();
			QString stampMD5{ QCryptographicHash::hash(QByteArray::number(timeStamp)
				, QCryptographicHash::Md5).toHex().toUpper() };
			token.clear();
			for (int i = 0; i < pwdMD5.length();i++)
			{
				token.append(pwdMD5[i]);
				token.append(stampMD5[i]);
			}
		}
	};
	void from_json(const nlohmann::json& j, TimeStampToken& token);
	void to_json(nlohmann::json& j, const TimeStampToken& token);
}

