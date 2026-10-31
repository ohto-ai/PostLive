#include "GraphicsCapture.h"

#include "../../core/Logging.h"

#include <QDateTime>

#include <d3d11.h>
#include <dxgi1_2.h>

// WinRT / Windows Graphics Capture (headers ship with the Windows SDK).
// Note: the D3D interop header declares the ABI `Windows::...` namespaces, so
// every cppwinrt name in this file is explicitly qualified with `winrt::`
// and every interop name with a leading `::`.
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Graphics.h>
#include <winrt/Windows.UI.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <windows.graphics.capture.interop.h>

#pragma comment(lib, "windowsapp.lib")
#pragma comment(lib, "d3d11.lib")

namespace
{
	namespace wfc = winrt::Windows::Foundation;
	namespace wgc = winrt::Windows::Graphics::Capture;
	namespace wgd = winrt::Windows::Graphics::DirectX;
	namespace interop = winrt::Windows::Graphics::DirectX::Direct3D11;

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

	bool osHasCaptureApis()
	{
		try
		{
			// WGC ships with UniversalApiContract v8 (Windows 10 1803).
			return wfc::Metadata::ApiInformation::IsApiContractPresent(
				winrt::hstring(L"Windows.Foundation.UniversalApiContract"), 8);
		}
		catch (...)
		{
			return false;
		}
	}

	// Win11 exposes TryCreateFromWindowId/DisplayId on the projection; Win10
	// needs the IGraphicsCaptureItemInterop activation path.
	wgc::GraphicsCaptureItem createItemForWindow(HWND hwnd)
	{
		if (wfc::Metadata::ApiInformation::IsMethodPresent(
			winrt::hstring(L"Windows.Graphics.Capture.GraphicsCaptureItem"),
			winrt::hstring(L"TryCreateFromWindowId")))
		{
			return wgc::GraphicsCaptureItem::TryCreateFromWindowId(
				winrt::Windows::UI::WindowId{ reinterpret_cast<std::uint64_t>(hwnd) });
		}
		auto interopFactory = winrt::get_activation_factory<wgc::GraphicsCaptureItem,
			IGraphicsCaptureItemInterop>();
		wgc::GraphicsCaptureItem item = nullptr;
		winrt::check_hresult(interopFactory->CreateForWindow(hwnd,
			winrt::guid_of<wgc::GraphicsCaptureItem>(), winrt::put_abi(item)));
		return item;
	}

	wgc::GraphicsCaptureItem createItemForMonitor(HMONITOR monitor)
	{
		if (wfc::Metadata::ApiInformation::IsMethodPresent(
			winrt::hstring(L"Windows.Graphics.Capture.GraphicsCaptureItem"),
			winrt::hstring(L"TryCreateFromDisplayId")))
		{
			return wgc::GraphicsCaptureItem::TryCreateFromDisplayId(
				winrt::Windows::Graphics::DisplayId{ reinterpret_cast<std::uint64_t>(monitor) });
		}
		auto interopFactory = winrt::get_activation_factory<wgc::GraphicsCaptureItem,
			IGraphicsCaptureItemInterop>();
		wgc::GraphicsCaptureItem item = nullptr;
		winrt::check_hresult(interopFactory->CreateForMonitor(monitor,
			winrt::guid_of<wgc::GraphicsCaptureItem>(), winrt::put_abi(item)));
		return item;
	}

	// Wraps the DXGI device into the WinRT IDirect3DDevice the frame pool wants.
	interop::IDirect3DDevice wrapDevice(ID3D11Device* rawDevice)
	{
		winrt::com_ptr<IDXGIDevice> dxgiDevice;
		if (FAILED(rawDevice->QueryInterface(__uuidof(IDXGIDevice), dxgiDevice.put_void())))
			return nullptr;
		IInspectable* rawInspectable = nullptr;
		if (FAILED(CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.get(), &rawInspectable)))
			return nullptr;
		winrt::com_ptr<IInspectable> inspectable;
		inspectable.copy_from(rawInspectable);
		return inspectable.as<interop::IDirect3DDevice>();
	}

	QImage textureToImage(ID3D11DeviceContext* context, ID3D11Texture2D* texture,
		ID3D11Texture2D* staging, int width, int height)
	{
		context->CopyResource(staging, texture);
		D3D11_MAPPED_SUBRESOURCE mapped{};
		if (FAILED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)))
			return QImage();
		QImage frame(width, height, QImage::Format_ARGB32);
		for (int y = 0; y < height; ++y)
			memcpy(frame.scanLine(y),
				static_cast<const BYTE*>(mapped.pData) + size_t(y) * mapped.RowPitch,
				size_t(width) * 4);
		context->Unmap(staging, 0);
		return frame;
	}
}

bool GraphicsCapture::isSupported()
{
	const bool supported = osHasCaptureApis();
	qCInfo(lcStream) << "WGC supported:" << supported;
	return supported;
}

GraphicsCapture::GraphicsCapture(QString sourceName, Target target, int fps, QObject* parent)
	: QThread(parent)
	, m_sourceName(std::move(sourceName))
	, m_target(target)
	, m_fps(fps > 0 ? fps : 30)
{
}

void GraphicsCapture::stop()
{
	requestInterruption();
	wait();
}

void GraphicsCapture::run()
{
	makeThreadDpiAware();
	try
	{
		winrt::init_apartment(winrt::apartment_type::multi_threaded);
	}
	catch (...)
	{
	}

	try
	{
		// D3D11 device on the default adapter; the capture item resolves the
		// right output internally.
		ID3D11Device* rawDevice = nullptr;
		ID3D11DeviceContext* rawContext = nullptr;
		HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
			0, nullptr, 0, D3D11_SDK_VERSION, &rawDevice, nullptr, &rawContext);
		if (FAILED(hr))
		{
			qCWarning(lcStream) << "WGC: D3D11CreateDevice failed" << Qt::hex << quint32(hr);
			rawContext->Release();
			return;
		}
		interop::IDirect3DDevice device = wrapDevice(rawDevice);
		if (!device)
		{
			qCWarning(lcStream) << "WGC: device interop wrap failed";
			rawContext->Release();
			return;
		}

		wgc::GraphicsCaptureItem item = nullptr;
		HWND hwnd = nullptr;
		if (m_target == Target::Window)
		{
			// Browser tab titles change constantly: keep retrying the title
			// lookup instead of giving up, so a matching window re-attaches.
			bool logged = false;
			while (!isInterruptionRequested() && hwnd == nullptr)
			{
				hwnd = FindWindowW(nullptr,
					reinterpret_cast<const wchar_t*>(m_sourceName.utf16()));
				if (hwnd == nullptr)
				{
					if (!logged)
					{
						qCDebug(lcStream) << "WGC: window not found, will retry:" << m_sourceName;
						logged = true;
					}
					Sleep(500);
				}
			}
			if (isInterruptionRequested())
			{
				rawContext->Release();
				return;
			}
			item = createItemForWindow(hwnd);
		}
		else
		{
			// Callback-safe lookup state: a captureless lambda cannot touch
			// members, so both the name and the result travel via lParam.
			struct MonitorLookup
			{
				const QString* name;
				HMONITOR result;
			} lookup{ &m_sourceName, nullptr };
			EnumDisplayMonitors(nullptr, nullptr,
				[](HMONITOR hMonitor, HDC, LPRECT, LPARAM lParam) -> BOOL
				{
					auto* lookup = reinterpret_cast<MonitorLookup*>(lParam);
					MONITORINFOEXW info{};
					info.cbSize = sizeof(info);
					if (GetMonitorInfoW(hMonitor, &info)
						&& QString::fromWCharArray(info.szDevice) == *lookup->name)
					{
						lookup->result = hMonitor;
						return FALSE;
					}
					return TRUE;
				}, reinterpret_cast<LPARAM>(&lookup));
			if (lookup.result == nullptr)
			{
				qCWarning(lcStream) << "WGC: monitor not found:" << m_sourceName;
				rawContext->Release();
				return;
			}
			item = createItemForMonitor(lookup.result);
		}
		if (!item)
		{
			qCWarning(lcStream) << "WGC: capture item creation failed";
			rawContext->Release();
			return;
		}

		const int width = item.Size().Width;
		const int height = item.Size().Height;
		D3D11_TEXTURE2D_DESC stagingDesc{};
		stagingDesc.Width = width;
		stagingDesc.Height = height;
		stagingDesc.MipLevels = 1;
		stagingDesc.ArraySize = 1;
		stagingDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		stagingDesc.SampleDesc.Count = 1;
		stagingDesc.Usage = D3D11_USAGE_STAGING;
		stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		ID3D11Texture2D* staging = nullptr;
		if (FAILED(rawDevice->CreateTexture2D(&stagingDesc, nullptr, &staging)))
		{
			qCWarning(lcStream) << "WGC: staging texture failed";
			rawContext->Release();
			return;
		}

		auto framePool = wgc::Direct3D11CaptureFramePool::CreateFreeThreaded(
			device, wgd::DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, item.Size());
		auto session = framePool.CreateCaptureSession(item);
		try
		{
			// Hide the yellow border on builds that support it.
			session.IsBorderRequired(false);
		}
		catch (...)
		{
		}
		session.IsCursorCaptureEnabled(true);
		session.StartCapture();
		qCInfo(lcStream) << "WGC capture started:" << m_sourceName << width << "x" << height;

		const qint64 frameInterval = 1000 / m_fps;
		while (!isInterruptionRequested())
		{
			const qint64 tick = QDateTime::currentMSecsSinceEpoch();

			if (m_target == Target::Window && IsWindow(hwnd) == FALSE)
			{
				qCDebug(lcStream) << "WGC: window gone, waiting for it to return:"
					<< m_sourceName;
				emit frameReady(QImage());
				Sleep(500);
				continue;
			}

			wgc::Direct3D11CaptureFrame frame = framePool.TryGetNextFrame();
			if (frame)
			{
				auto surface = frame.Surface();
				auto access = surface.as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
				ID3D11Texture2D* texture = nullptr;
				if (SUCCEEDED(access->GetInterface(__uuidof(ID3D11Texture2D),
					reinterpret_cast<void**>(&texture))))
				{
					QImage image = textureToImage(rawContext, texture, staging,
						width, height);
					texture->Release();
					if (!image.isNull())
						emit frameReady(image);
				}
				frame.Close();
			}

			const qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - tick;
			if (elapsed < frameInterval)
				Sleep(DWORD(frameInterval - elapsed));
		}

		try
		{
			session.Close();
			framePool.Close();
		}
		catch (...)
		{
		}
		staging->Release();
		rawContext->Release();
		rawDevice->Release();
		qCInfo(lcStream) << "WGC capture stopped:" << m_sourceName;
	}
	catch (const winrt::hresult_error& e)
	{
		qCWarning(lcStream) << "WGC failed:" << QString::fromWCharArray(e.message().c_str());
	}
	catch (...)
	{
		qCWarning(lcStream) << "WGC failed with an unknown exception";
	}
}
