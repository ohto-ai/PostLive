#pragma once

#include <QEvent>
#include <QSystemTrayIcon>
#include <QTimer>
#include "SceneEncoder.h"
#include "ScenePreview.h"
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
	QString effectiveMicrophoneName() const;

	ScenePreview previewWidget{ this };
	SceneEncoder sceneEncoder{ this };
	QList<StreamSource> m_scene;
	bool m_sceneSeeded{ false };
	bool m_streaming{ false };
	QTimer deviceRefreshTimer{ this };

	bool m_smokeStream{ false }; // POSTLIVE_SMOKE_STREAM: auto-start for e2e tests
private:
	Ui::LivePlatformClass ui;
};
