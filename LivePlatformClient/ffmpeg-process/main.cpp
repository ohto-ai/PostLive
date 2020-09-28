#include <QtCore/QCoreApplication>
#include <QProcess>
#include <QDebug>
#include <QCameraInfo>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    auto list = QCameraInfo::availableCameras();
    for (auto& cam : list)
        qDebug() << cam.description();

    QProcess* process = new QProcess();
    QObject::connect(process, &QProcess::started, [&]
        {
            qDebug() << "ffmpeg started.";
        }
    );
    QObject::connect(process, static_cast<void(QProcess::*)(int)>(&QProcess::finished) , [&](int code)
        {
            qDebug() << "ffmpeg finished.Return " << code;
        }
    );

    QObject::connect(process, &QProcess::stateChanged, [&](QProcess::ProcessState state)
        {
            qDebug() << state;
        }
    );
    // ×ÀÃæ
    // 
    auto command = QString::asprintf(
        R"(ffmpeg.exe -f dshow -i video="%s" -i %s -filter_complex "overlay=5:5" -f dshow -i audio="%s" -f %s -s %dx%d %s)"
        , list[0].description().toStdString().c_str()
        , "thatboylogo.png"
        , "virtual-audio-capturer"
        , "flv"
        , 1920, 1080
        , "rtmp://localhost:1935/hls/home");

    qDebug() << command;
    puts(command.toStdString().c_str());
    //process->setWorkingDirectory(QCoreApplication::applicationDirPath());
    process->start(command);


     //R"(ffmpeg.exe -f gdigrab -i desktop -i thatboylogo.png -filter_complex "overlay=5:5" -f dshow -i audio="virtual-audio-capturer" -f flv -s 1920x1080 rtmp://localhost:1935/hls/home)");
    //process->start(R"(ffmpeg.exe)");
    if (!process->waitForStarted())
    {
        qDebug() << "failure!";
    }
    else
    {
        qDebug() << "succ!";
    }
    return a.exec();
}
