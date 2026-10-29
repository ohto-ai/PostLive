#pragma once
// Video preview surface: renders RGB32 frames from FFmpegWidget's decode
// thread, shows a loading animation while idle, reports errors inline and
// accepts file drops to switch source. Ported from PostLive.Re
// (PostLiveClient/ffmpeg/FFmpegWidget); the automatic QMainWindow status bar
// discovery was replaced by an opt-in setter so embedding into a frameless
// window does not lazily create one.
#include "../widgets/FileDropZone.h"

#include <QImage>
#include <QMovie>

class FFmpegVideo;

class FFmpegWidget : public FileDropZone
{
	Q_OBJECT

public:
	FFmpegWidget(QWidget* parent = nullptr);
	~FFmpegWidget() override;

	bool setInputDevice(const struct FFmpegInputDevice* device);
	void play();
	void stop();
	void addOption(const QString& key, const QString& value);

	// Optional: route decode status/errors to a status bar.
	void setStatusStatusBar(class QStatusBar* statusBar);

protected:
	void paintEvent(QPaintEvent* event) override;

protected slots:
	void setFrame(const QImage& frame);
	void clearFrame();
	void updateStatusBar();

private:
	bool option_displayFPS = false;

	int ffmpegVideoErrorCode = 0;
	QString ffmpegVideoErrorString;

	class QStatusBar* statusStatusBar = nullptr;
	FFmpegVideo* ffmpegVideo = nullptr;
	QImage frameImage;
	qreal decodeFPS = 0.0;

	bool frameAvailable = false;
	QMovie movie{ ":/main/res/splash.gif" };
};
