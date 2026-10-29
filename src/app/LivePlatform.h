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
	void startPreview();
	void stopPreview();
	void startStreaming();
	void stopStreaming();
	void updateStreamControls(bool streaming);
	void appendStatus(const QString& text);

	const QString ffmpegPath{ R"(ffmpeg.exe)" };
	const QString videoFormt{ "flv" };

	QString logoImage{ "assets/pilipili-logo.png" };
	QCamera* viewCamera{ nullptr };
	QCameraViewfinder cameraViewfinder{ this };
	QProcess ffmpegProcess{ this };

	bool stopRequested{ false };
	bool errorNotified{ false };
	QString lastErrorOutput;
private:
	Ui::LivePlatformClass ui;
};
