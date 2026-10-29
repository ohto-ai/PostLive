#pragma once

#include <QEvent>
#include <QProcess>
#include <QSystemTrayIcon>
#include <QTimer>
#include "FFmpegWidget.h"
#include "FFmpegInputDevice.h"
#include "StreamArgs.h"
#include "utils.h"
#include "DragableMainWindow.h"
#include "ui_LivePlatform.h"

class LivePlatform : public DragableMainWindow
{
	Q_OBJECT

public:
	LivePlatform(QWidget* parent = Q_NULLPTR);
	~LivePlatform() override;

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
	void refreshWindowList();
	void applySourceMode();
	CaptureSource currentSource() const;
	QString cameraDeviceName() const;

	const QString ffmpegPath{ R"(ffmpeg.exe)" };

	QString logoImage{ "assets/pilipili-logo.png" };
	FFmpegWidget previewWidget{ this };
	// Backing store whose address is handed to the decoder; the decoder only
	// keeps the pointer, so the device must outlive the preview session.
	FFmpegInputDevice currentDevice;
	QString localFilePath;
	QProcess ffmpegProcess{ this };
	QTimer deviceRefreshTimer{ this };

	bool stopRequested{ false };
	bool errorNotified{ false };
	QString lastErrorOutput;
private:
	Ui::LivePlatformClass ui;
};
