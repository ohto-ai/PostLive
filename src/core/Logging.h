#pragma once

#include <QLoggingCategory>

// Category rules: use the narrowest category that fits, so log files can be
// filtered per subsystem. Keep messages ASCII-friendly where possible.
Q_DECLARE_LOGGING_CATEGORY(lcApp)      // application lifecycle, global errors
Q_DECLARE_LOGGING_CATEGORY(lcCore)     // config, user data, token storage
Q_DECLARE_LOGGING_CATEGORY(lcNetwork)  // authentication requests
Q_DECLARE_LOGGING_CATEGORY(lcStream)   // FFmpeg streaming
Q_DECLARE_LOGGING_CATEGORY(lcDevice)   // camera / microphone devices

namespace thatboy
{
	namespace log
	{
		// Installs a message handler that appends every Qt message to
		// <directory>/PostLive.log (rotated at kMaxLogBytes) and mirrors it
		// to stderr. Call once after QApplication is created.
		void initLogging(const QString& directory = QStringLiteral("log"));
	}
}
