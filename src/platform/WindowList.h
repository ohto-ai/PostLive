#pragma once
// Enumerates capturable top-level windows. gdigrab identifies a window only
// by its exact title, so capture sources carry the title string; windows
// with duplicate titles cannot be told apart by ffmpeg.
#include <QList>
#include <QString>

struct WindowInfo
{
	QString title;
	void* handle; // HWND, kept for future use / de-duplication; not passed to ffmpeg
};

// Visible, non-minimized, non-tool top-level windows with a title.
// Excludes the desktop (Progman) and cloaked UWP windows.
QList<WindowInfo> visibleTopLevelWindows();
