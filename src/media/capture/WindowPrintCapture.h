#pragma once
// Window capture via PrintWindow with PW_RENDERFULLCONTENT: unlike GDI
// BitBlt (ffmpeg gdigrab), this captures hardware-accelerated windows
// (Chrome, Electron, ...) on Windows 8.1+.
#include <QImage>
#include <QThread>
#include <QString>

class WindowPrintCapture : public QThread
{
	Q_OBJECT

public:
	WindowPrintCapture(QString windowTitle, int fps, QObject* parent = nullptr);
	void run() override;
	void stop();

signals:
	void frameReady(const QImage& frame);

private:
	QString m_windowTitle;
	int m_fps;
};
