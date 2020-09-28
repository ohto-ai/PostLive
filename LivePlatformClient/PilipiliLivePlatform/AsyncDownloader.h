#pragma once
#include <QFile>
#include <QByteArray>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>

class AsyncDownloader
	: public QObject
{
	Q_OBJECT
public:
	AsyncDownloader(QObject* parent = nullptr);
	~AsyncDownloader();

	void download(QString url, QString file);
protected:
	std::atomic_int downloadCount{ 0 };
	void saveResource(QString url, QNetworkReply& reply, QString file);
signals:
	void downloadCountChanged(int count);
	void finished(QString url, QString file, bool success);
	void error(QString url, QString file, QNetworkReply::NetworkError);
};
