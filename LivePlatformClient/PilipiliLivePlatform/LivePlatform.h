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
	const QString ffmpegPath{ R"(bin\ffmpeg.exe)" };
	const QString videoFormt{ "flv" };
	
	QString logoImage{ "assets\\pilipili-logo.png" };
	QCamera* viewCamera{ nullptr };
	QCameraViewfinder cameraViewfinder{ this };
	QProcess ffmpegProcess{ this };
private:
	Ui::LivePlatformClass ui;
};
