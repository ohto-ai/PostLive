#include "DesktopDuplicationCapture.h"

#include "../../core/Logging.h"

#include <QDateTime>
#include <QPainter>

#include <d3d11.h>
#include <dxgi1_2.h>

namespace
{
	constexpr UINT AcquireTimeoutMs = 100;

	// Per-monitor DPI context for the capture thread only, so Win32
	// coordinates stay in physical pixels without changing the UI thread.
	void makeThreadDpiAware()
	{
		using SetThreadDpiAwarenessContextFn = void*(WINAPI*)(void*);
		if (HMODULE user32 = GetModuleHandleW(L"user32.dll"))
		{
			auto setContext = reinterpret_cast<SetThreadDpiAwarenessContextFn>(
				GetProcAddress(user32, "SetThreadDpiAwarenessContext"));
			if (setContext)
				setContext(reinterpret_cast<void*>(-4)); // PER_MONITOR_AWARE_V2
		}
	}

	// DXGI duplication never includes the mouse cursor; draw a simple arrow
	// pointer manually (OBS draws the real cursor sprite, this is the cheap
	// approximation).
	void drawCursor(QImage& frame, const QString& monitorName)
	{
		CURSORINFO cursor{};
		cursor.cbSize = sizeof(cursor);
		if (!GetCursorInfo(&cursor) || !(cursor.flags & CURSOR_SHOWING))
			return;
		MONITORINFOEXW monitorInfo{};
		monitorInfo.cbSize = sizeof(monitorInfo);
		if (!GetMonitorInfoW(MonitorFromPoint(cursor.ptScreenPos, MONITOR_DEFAULTTONEAREST), &monitorInfo)
			|| QString::fromWCharArray(monitorInfo.szDevice) != monitorName)
			return;

		QPainter painter(&frame);
		const QPoint tip(cursor.ptScreenPos.x - monitorInfo.rcMonitor.left,
			cursor.ptScreenPos.y - monitorInfo.rcMonitor.top);
		QPolygonF arrow;
		arrow << QPointF(0, 0) << QPointF(0, 17) << QPointF(4.2, 13.4)
			<< QPointF(7.5, 20.2) << QPointF(10, 19) << QPointF(6.9, 12.2) << QPointF(12, 12);
		painter.setPen(QPen(Qt::black, 1.4));
		painter.setBrush(Qt::white);
		painter.drawPolygon(arrow.translated(tip));
	}
}

QList<DesktopDuplicationCapture::MonitorInfo> DesktopDuplicationCapture::availableMonitors()
{
	makeThreadDpiAware();
	QList<MonitorInfo> monitors;
	EnumDisplayMonitors(
		nullptr, nullptr,
		[](HMONITOR hMonitor, HDC, LPRECT, LPARAM lParam) -> BOOL
		{
			auto* list = reinterpret_cast<QList<MonitorInfo>*>(lParam);
			MONITORINFOEXW info{};
			info.cbSize = sizeof(info);
			if (GetMonitorInfoW(hMonitor, &info))
			{
				MonitorInfo entry;
				entry.deviceName = QString::fromWCharArray(info.szDevice);
				entry.geometry = QRect(info.rcMonitor.left, info.rcMonitor.top,
					info.rcMonitor.right - info.rcMonitor.left,
					info.rcMonitor.bottom - info.rcMonitor.top);
				list->append(entry);
			}
			return TRUE;
		},
		reinterpret_cast<LPARAM>(&monitors));
	return monitors;
}

DesktopDuplicationCapture::DesktopDuplicationCapture(QString monitorDeviceName, int fps, QObject* parent)
	: QThread(parent)
	, m_monitorName(std::move(monitorDeviceName))
	, m_fps(fps > 0 ? fps : 30)
{
}

void DesktopDuplicationCapture::stop()
{
	requestInterruption();
	wait();
	shutdownOutput();
}

bool DesktopDuplicationCapture::initOutput()
{
	shutdownOutput();

	IDXGIFactory1* factory = nullptr;
	if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory))))
		return false;

	IDXGIAdapter1* adapter = nullptr;
	IDXGIOutput* output = nullptr;
	for (UINT a = 0; factory->EnumAdapters1(a, &adapter) == S_OK && !output; ++a)
	{
		for (UINT o = 0; adapter->EnumOutputs(o, &output) == S_OK && !output; ++o)
		{
			DXGI_OUTPUT_DESC desc{};
			output->GetDesc(&desc);
			if (desc.AttachedToDesktop
				&& QString::fromWCharArray(desc.DeviceName) == m_monitorName)
			{
				m_width = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
				m_height = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;
			}
			else
			{
				output->Release();
				output = nullptr;
			}
		}
		if (!output)
			adapter->Release();
	}
	factory->Release();

	if (!output || m_width <= 0 || m_height <= 0)
	{
		if (output) output->Release();
		if (adapter) adapter->Release();
		return false;
	}

	IDXGIOutput1* output1 = nullptr;
	const HRESULT duplicateQuery = output->QueryInterface(__uuidof(IDXGIOutput1),
		reinterpret_cast<void**>(&output1));
	output->Release();
	output = nullptr;
	if (FAILED(duplicateQuery))
	{
		if (adapter) adapter->Release();
		return false;
	}

	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* context = nullptr;
	const HRESULT deviceCreated = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr,
		0, nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, &context);
	if (adapter) adapter->Release();
	if (FAILED(deviceCreated))
	{
		output1->Release();
		return false;
	}

	IDXGIOutputDuplication* duplication = nullptr;
	HRESULT hr = output1->DuplicateOutput(device, &duplication);
	output1->Release();
	if (FAILED(hr))
	{
		device->Release();
		context->Release();
		return false;
	}

	D3D11_TEXTURE2D_DESC stagingDesc{};
	stagingDesc.Width = m_width;
	stagingDesc.Height = m_height;
	stagingDesc.MipLevels = 1;
	stagingDesc.ArraySize = 1;
	stagingDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	stagingDesc.SampleDesc.Count = 1;
	stagingDesc.Usage = D3D11_USAGE_STAGING;
	stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

	ID3D11Texture2D* staging = nullptr;
	if (FAILED(device->CreateTexture2D(&stagingDesc, nullptr, &staging)))
	{
		duplication->Release();
		device->Release();
		context->Release();
		return false;
	}

	m_device = device;
	m_context = context;
	m_output1 = output1;
	m_duplication = duplication;
	m_staging = staging;
	qCDebug(lcStream) << "desktop duplication started:" << m_monitorName << m_width << "x" << m_height;
	return true;
}

void DesktopDuplicationCapture::shutdownOutput()
{
	if (m_staging) static_cast<ID3D11Texture2D*>(m_staging)->Release();
	if (m_duplication) static_cast<IDXGIOutputDuplication*>(m_duplication)->Release();
	if (m_output1) static_cast<IDXGIOutput1*>(m_output1)->Release();
	if (m_output) static_cast<IDXGIOutput*>(m_output)->Release();
	if (m_context) static_cast<ID3D11DeviceContext*>(m_context)->Release();
	if (m_device) static_cast<ID3D11Device*>(m_device)->Release();
	m_staging = m_duplication = m_output1 = m_output = m_context = m_device = nullptr;
}

// Returns the captured frame, or a null image when there was no update.
QImage DesktopDuplicationCapture::grabFrame()
{
	auto* duplication = static_cast<IDXGIOutputDuplication*>(m_duplication);
	auto* context = static_cast<ID3D11DeviceContext*>(m_context);
	auto* staging = static_cast<ID3D11Texture2D*>(m_staging);

	DXGI_OUTDUPL_FRAME_INFO frameInfo{};
	IDXGIResource* resource = nullptr;
	HRESULT hr = duplication->AcquireNextFrame(AcquireTimeoutMs, &frameInfo, &resource);
	if (hr == DXGI_ERROR_WAIT_TIMEOUT)
		return QImage(); // no update; the previous frame stays valid
	if (FAILED(hr))
	{
		// Access lost (mode change, fullscreen transition): re-init next tick.
		qCDebug(lcStream) << "desktop duplication reinit:" << m_monitorName << Qt::hex << quint32(hr);
		shutdownOutput();
		return QImage();
	}

	QImage frame;
	ID3D11Texture2D* texture = nullptr;
	if (SUCCEEDED(resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&texture))))
	{
		context->CopyResource(staging, texture);
		D3D11_MAPPED_SUBRESOURCE mapped{};
		if (SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)))
		{
			frame = QImage(m_width, m_height, QImage::Format_ARGB32);
			for (int y = 0; y < m_height; ++y)
				memcpy(frame.scanLine(y),
					static_cast<const BYTE*>(mapped.pData) + size_t(y) * mapped.RowPitch,
					size_t(m_width) * 4);
			context->Unmap(staging, 0);
		}
		texture->Release();
	}
	resource->Release();
	duplication->ReleaseFrame();
	return frame;
}

void DesktopDuplicationCapture::run()
{
	makeThreadDpiAware();

	const qint64 frameInterval = 1000 / m_fps;
	while (!isInterruptionRequested())
	{
		const qint64 tick = QDateTime::currentMSecsSinceEpoch();

		if (m_duplication == nullptr && !initOutput())
		{
			msleep(500);
			continue;
		}

		QImage frame = grabFrame();
		if (!frame.isNull())
		{
			drawCursor(frame, m_monitorName);
			emit frameReady(frame);
		}

		const qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - tick;
		if (elapsed < frameInterval)
			msleep(DWORD(frameInterval - elapsed));
	}

	shutdownOutput();
}
