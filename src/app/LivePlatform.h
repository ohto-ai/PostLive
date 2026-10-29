#pragma once

#include <QEvent>
#include <QProcess>
#include <QCameraInfo>
#include <QCameraViewfinder>
#include <QSystemTrayIcon>
#include <QTimer>
#include "utils.h"
#include "DragableMainWindow.h"
#include "ui_LivePlatform.h"

class LivePlatform : public DragableMainWindow
{
	Q_OBJECT

public:
	LivePlatform(QWidget* parent = Q_NULLPTR);
	~LivePlatform() override;

	void setViewCamera(const QCameraInfo& cameraInfo);

protected:
	void changeEvent(QEvent* event) override;
	void showFromTray();
	void hideToTray();
	void startPreview();
	void stopPreview();
	void startStreaming();
	void stopStreaming();
	void updateStreamControls(bool streaming);
	void appendStatus(const QString& text);
	void refreshDevices();

	const QString ffmpegPath{ R"(ffmpeg.exe)" };
	const QString videoFormt{ "flv" };

	QString logoImage{ "assets/pilipili-logo.png" };
	QCamera* viewCamera{ nullptr };
	QCameraViewfinder cameraViewfinder{ this };
	QProcess ffmpegProcess{ this };
	QTimer deviceRefreshTimer{ this };

	bool stopRequested{ false };
	bool errorNotified{ false };
	QString lastErrorOutput;
private:
	Ui::LivePlatformClass ui;
};
