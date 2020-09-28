#pragma once

#include <QProcess>
#include <QCameraInfo>
#include <QCameraViewfinder>
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
	QCamera* viewCamera{ nullptr };
	QCameraViewfinder cameraViewfinder{ this };
protected:
	QProcess ffmpegProcess{ this };
private:
	Ui::LivePlatformClass ui;
};
