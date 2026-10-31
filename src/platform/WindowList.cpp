#include "WindowList.h"

#include <dwmapi.h>
#include <windows.h>

namespace
{
	using DwmGetWindowAttributeFn = HRESULT(WINAPI*)(HWND, DWORD, PVOID, DWORD);
}

QList<WindowInfo> visibleTopLevelWindows()
{
	QList<WindowInfo> result;

	EnumWindows(
		[](HWND hwnd, LPARAM lParam) -> BOOL
		{
			auto* result = reinterpret_cast<QList<WindowInfo>*>(lParam);

			// Cloaked UWP windows are visible but not actually rendered on
			// screen; resolve DwmGetWindowAttribute dynamically so dwmapi
			// stays an optional dependency.
			static const DwmGetWindowAttributeFn dwmGetWindowAttribute = []
				{
					if (HMODULE dwm = GetModuleHandleW(L"dwmapi.dll"))
						return reinterpret_cast<DwmGetWindowAttributeFn>(
							GetProcAddress(dwm, "DwmGetWindowAttribute"));
					return static_cast<DwmGetWindowAttributeFn>(nullptr);
				}();

			if (!IsWindowVisible(hwnd) || IsIconic(hwnd))
				return TRUE;

			const LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
			if (exStyle & WS_EX_TOOLWINDOW)
				return TRUE;

			wchar_t className[64]{};
			if (GetClassNameW(hwnd, className, 64) && wcscmp(className, L"Progman") == 0)
				return TRUE; // the desktop window; gdigrab "desktop" covers it

			const int length = GetWindowTextLengthW(hwnd);
			if (length <= 0)
				return TRUE;

			if (dwmGetWindowAttribute)
			{
				DWORD cloaked = 0;
				if (SUCCEEDED(dwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked)))
					&& cloaked != 0)
					return TRUE;
			}

			QString title;
			title.resize(length + 1);
			const int written = GetWindowTextW(hwnd,
				reinterpret_cast<wchar_t*>(title.data()), length + 1);
			title.truncate(qMax(0, written));
			if (title.isEmpty())
				return TRUE;

			result->append({ title, hwnd });
			return TRUE;
		},
		reinterpret_cast<LPARAM>(&result));

	return result;
}
