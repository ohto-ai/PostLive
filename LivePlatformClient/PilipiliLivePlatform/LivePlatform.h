#pragma once

#include <QProcess>
#include <QCameraInfo>
#include <QCameraViewfinder>
#include <QSystemTrayIcon>
#include "utils.h"
#include "DragableMainWindow.h"
#include "ui_LivePlatform.h"

class LivePlatform : public DragableMainWindow
{
	Q_OBJECT

public:
	LivePlatform(QWidget* parent = Q_NULLPTR);

	void setViewCamera(const QCameraInfo& cameraInfo);
protected:
	QString commandFormt{ R"(%s -f %s -i video="%s" -i %s -filter_complex "overlay=5:5" -f dshow -i audio="%s" -f %s -s %dx%d rtmp://%s/%s)" };
	QString ffmpegPath{ R"(bin\ffmpeg.exe)" };
	QString inputCamera{ "dshow" };
	QString logoImage{ "assets\\thatboylogo.png" };
	QString inputAudio{ "virtual-audio-capturer" };
	QString videoFormt{ "flv" };
	QSize videoSize{ 1920,1080 };
	QString streamPath{ "localhost:1935/hls" };
	QCamera* viewCamera{ nullptr };
	QCameraViewfinder cameraViewfinder{ this };
protected:
	QProcess ffmpegProcess{ this };
private:
	Ui::LivePlatformClass ui;
};
