#pragma once
// Desktop duplication (DXGI) frame source: one instance captures one monitor
// at ~fps, emitting BGRA QImages. Handles output re-init on access loss.
// Must be used from its own thread (it is a QThread).
#include <QImage>
#include <QList>
#include <QRect>
#include <QThread>
#include <QString>

class DesktopDuplicationCapture : public QThread
{
	Q_OBJECT

public:
	struct MonitorInfo
	{
		QString deviceName; // "\\.\DISPLAY1" as reported by DXGI / EnumDisplayMonitors
		QRect geometry;     // virtual-desktop coordinates
	};

	static QList<MonitorInfo> availableMonitors();

	DesktopDuplicationCapture(QString monitorDeviceName, int fps, QObject* parent = nullptr);
	void run() override;
	void stop();

signals:
	void frameReady(const QImage& frame);

private:
	bool initOutput();
	void shutdownOutput();
	QImage grabFrame();

	QString m_monitorName;
	int m_fps;
	void* m_device = nullptr;         // ID3D11Device*
	void* m_context = nullptr;        // ID3D11DeviceContext*
	void* m_output1 = nullptr;        // IDXGIOutput1*
	void* m_output = nullptr;         // IDXGIOutput*
	void* m_duplication = nullptr;    // IDXGIOutputDuplication*
	void* m_staging = nullptr;        // ID3D11Texture2D* (staging)
	int m_width = 0;
	int m_height = 0;
};
