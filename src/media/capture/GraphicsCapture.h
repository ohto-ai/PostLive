#pragma once
// Windows Graphics Capture frame source (the OBS "Windows Graphics Capture"
// method): captures monitors and hardware-accelerated windows (Chrome,
// Electron, flip-model games) reliably where GDI-based approaches return
// black frames. Windows 10 1903+.
//
// Polling design: the capture thread pulls frames from the frame pool at
// ~fps instead of wiring FrameArrived coroutines, which keeps the thread
// lifecycle trivial.
#include <QImage>
#include <QThread>
#include <QString>

class GraphicsCapture : public QThread
{
	Q_OBJECT

public:
	enum class Target
	{
		Monitor,  // name = "\\.\DISPLAYn" device name
		Window,   // name = exact window title
	};

	// True on Windows 10 1903+ with the GraphicsCapture APIs present.
	static bool isSupported();

	GraphicsCapture(QString sourceName, Target target, int fps, QObject* parent = nullptr);
	void run() override;
	void stop();

signals:
	void frameReady(const QImage& frame);

private:
	QString m_sourceName;
	Target m_target;
	int m_fps;
};
