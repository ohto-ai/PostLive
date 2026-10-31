#include "WindowPrintCapture.h"

#include "../../core/Logging.h"

#include <QDateTime>
#include <QPainter>

#include <windows.h>

namespace
{
	constexpr UINT RenderFullContent = 0x00000002; // PW_RENDERFULLCONTENT

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

	QImage captureWindow(HWND hwnd)
	{
		RECT client{};
		if (!GetClientRect(hwnd, &client))
			return QImage();
		const int width = client.right - client.left;
		const int height = client.bottom - client.top;
		if (width <= 0 || height <= 0)
			return QImage();

		HDC windowDc = GetDC(hwnd);
		HDC memoryDc = CreateCompatibleDC(windowDc);
		HBITMAP bitmap = CreateCompatibleBitmap(windowDc, width, height);
		HGDIOBJ previous = SelectObject(memoryDc, bitmap);

		QImage frame;
		// PW_RENDERFULLCONTENT asks DWM for the full (accelerated) content.
		if (PrintWindow(hwnd, memoryDc, RenderFullContent))
		{
			BITMAPINFO info{};
			info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			info.bmiHeader.biWidth = width;
			info.bmiHeader.biHeight = -height; // top-down
			info.bmiHeader.biPlanes = 1;
			info.bmiHeader.biBitCount = 32;
			info.bmiHeader.biCompression = BI_RGB;
			frame = QImage(width, height, QImage::Format_ARGB32);
			if (!GetDIBits(memoryDc, bitmap, 0, UINT(height),
				frame.bits(), &info, DIB_RGB_COLORS))
				frame = QImage();
		}

		SelectObject(memoryDc, previous);
		DeleteObject(bitmap);
		DeleteDC(memoryDc);
		ReleaseDC(hwnd, windowDc);
		return frame;
	}
}

WindowPrintCapture::WindowPrintCapture(QString windowTitle, int fps, QObject* parent)
	: QThread(parent)
	, m_windowTitle(std::move(windowTitle))
	, m_fps(fps > 0 ? fps : 30)
{
}

void WindowPrintCapture::stop()
{
	requestInterruption();
	wait();
}

void WindowPrintCapture::run()
{
	makeThreadDpiAware();

	HWND hwnd = FindWindowW(nullptr, reinterpret_cast<const wchar_t*>(m_windowTitle.utf16()));
	bool contentAvailable = false;
	const qint64 frameInterval = 1000 / m_fps;

	while (!isInterruptionRequested())
	{
		const qint64 tick = QDateTime::currentMSecsSinceEpoch();

		// The window can be closed and reopened under the same title.
		if (IsWindow(hwnd) == FALSE)
		{
			hwnd = FindWindowW(nullptr, reinterpret_cast<const wchar_t*>(m_windowTitle.utf16()));
			contentAvailable = false;
		}

		if (hwnd != nullptr)
		{
			QImage frame = captureWindow(hwnd);
			if (!frame.isNull())
			{
				contentAvailable = true;
				emit frameReady(frame);
			}
			else if (contentAvailable)
			{
				contentAvailable = false;
				qCDebug(lcStream) << "window capture returned nothing:" << m_windowTitle;
				emit frameReady(QImage());
			}
		}
		else if (contentAvailable)
		{
			contentAvailable = false;
			qCDebug(lcStream) << "window gone:" << m_windowTitle;
			emit frameReady(QImage());
		}

		const qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - tick;
		if (elapsed < frameInterval)
			msleep(DWORD(frameInterval - elapsed));
	}
}
