#pragma once

#include <QEvent>
#include <QProcess>
#include <QSystemTrayIcon>
#include <QTimer>
#include "ScenePreview.h"
#include "StreamArgs.h"
#include "StreamSource.h"
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
	void startStreaming();
	void stopStreaming();
	void updateStreamControls(bool streaming);
	void appendStatus(const QString& text);
	void refreshDevices();
	void openSettings();

	// Scene management: the list order is the layer order (first = base).
	void addSource(const StreamSource& source);
	void removeSource();
	void moveSource(int delta);
	void syncSourceList();
	void syncPreview();
	void persistScene();

	QString pickWindowTitle();
	QString cameraDeviceName() const;

	const QString ffmpegPath{ R"(ffmpeg.exe)" };

	QString logoImage{ "assets/pilipili-logo.png" };
	ScenePreview previewWidget{ this };
	QList<StreamSource> m_scene;
	bool m_sceneSeeded{ false };
	QProcess ffmpegProcess{ this };
	QTimer deviceRefreshTimer{ this };

	bool stopRequested{ false };
	bool errorNotified{ false };
	QString lastErrorOutput;
private:
	Ui::LivePlatformClass ui;
};
