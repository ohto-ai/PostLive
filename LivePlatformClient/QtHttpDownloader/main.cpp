#include <QtCore/QCoreApplication>
#include "AsyncDownloader.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    AsyncDownloader downloader;
    downloader.download("http://thatboy.info:5120/uploads/medium/6ace02fe577e2ddf8c8736cdf6965374.jpg", "test0.jpg");
    downloader.download("http://thatboy.info:5120/uploads/medium/6ace02fe577e2ddf8c8736cdf6965374.jp", "test1.jpg");
    downloader.download("http://thatboy.info:5120/uploads/medium/6ace02fe577e2ddf8c8736cdf6965374.jpg", "test2.jpg");
    downloader.download("http://thatboy.info:5120/uploads/medium/6ace02fe577e2ddf8c8736cdf6965374.jpg", "test3.jpg");
    QObject::connect(&downloader, &AsyncDownloader::finished, [=](QString url, QString file, bool success)
        {
            qDebug() << file << " download " << (success ? "success" : "failed");
        }); 
    QObject::connect(&downloader, &AsyncDownloader::downloadCountChanged, [=](int count)
        {
            qDebug() << count << " download remain.";
        });
    QObject::connect(&downloader, &AsyncDownloader::error, [=](QString _t1, QString _t2, QNetworkReply::NetworkError _t3)
        {
            qDebug() << _t2 << " download error.";
        });
    return a.exec();
}