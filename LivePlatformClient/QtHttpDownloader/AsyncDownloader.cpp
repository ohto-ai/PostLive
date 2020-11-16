#include "AsyncDownloader.h"

AsyncDownloader::AsyncDownloader(QObject* parent)
	: QObject(parent)
{}

AsyncDownloader::~AsyncDownloader()
{}

void AsyncDownloader::download(QString url, QString file)
{
    ++downloadCount;
    emit downloadCountChanged(downloadCount);
    QNetworkRequest request{ url };
    auto reply = (new QNetworkAccessManager(this))->get(request);
    QObject::connect(reply, &QNetworkReply::finished
        , std::bind(&AsyncDownloader::saveResource, this, url, std::ref(*reply), file));
    QObject::connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error)
        , [=](QNetworkReply::NetworkError e)
        {
            emit error(url, file, e);
        });
}

void AsyncDownloader::saveResource(QString url, QNetworkReply& reply, QString filePath)
{
    if (reply.error() == QNetworkReply::NoError)
    {
        QFile& file = *new QFile(filePath);
        if (file.open(QIODevice::WriteOnly))
        {
            file.write(reply.readAll());
            file.close();
        }
    }
    emit finished(url, filePath, reply.error() == QNetworkReply::NoError);
    --downloadCount;
    emit downloadCountChanged(downloadCount);
}