#pragma execution_character_set("utf-8")
#include "LivePlatform.h"
#include "LiveLoginDialog.h"
#include "Logging.h"
#include <QtWidgets/QApplication>
#include <QDir>
#include <QFile>
#include <QMovie>
#include <QNetworkProxyFactory>
#include <QTimer>
#include <QSplashScreen>
#include <windows.h>
#include <DbgHelp.h>

#pragma comment(lib, "Dbghelp.lib")

#ifndef POSTLIVE_VERSION
#define POSTLIVE_VERSION "0.0.0-dev"
#endif

int GenerateDump(EXCEPTION_POINTERS* pExceptionPointers)
{
    // 创建Dump文件
    HANDLE hDumpFile = CreateFile(L"PostLive.dmp", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hDumpFile != INVALID_HANDLE_VALUE)
	{
        // Dump信息
        MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
        dumpInfo.ExceptionPointers = pExceptionPointers;
        dumpInfo.ThreadId = GetCurrentThreadId();
        dumpInfo.ClientPointers = TRUE;

        // 写入Dump文件
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpNormal, &dumpInfo, NULL, NULL);
        CloseHandle(hDumpFile);
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

LONG WINAPI ExceptionFilter(EXCEPTION_POINTERS* pExceptionPointers)
{
	if (IsDebuggerPresent()) {
		qCCritical(lcApp) << "Unhandled exception; a debugger is attached, letting it take over.";
		return EXCEPTION_CONTINUE_SEARCH;
	}
	else {
		qCCritical(lcApp) << "Unhandled exception; the program will exit. Writing minidump...";
        return GenerateDump(pExceptionPointers);
	}
}

int main(int argc, char* argv[])
{
	QApplication a(argc, argv);
	QNetworkProxyFactory::setUseSystemConfiguration(true);
	QCoreApplication::setApplicationName("PostLive");
	QCoreApplication::setOrganizationName("ohto-ai");
	QCoreApplication::setApplicationVersion(POSTLIVE_VERSION);

	// Resolve application.json, users.json, assets/, user/ and ffmpeg.exe
	// relative to the executable, regardless of how the process was started.
	QDir::setCurrent(QCoreApplication::applicationDirPath());

	thatboy::log::initLogging();
	qCInfo(lcApp) << "PostLive" << POSTLIVE_VERSION << "starting";

	// Global theme extracted from the PostLive.Re UI design.
	QFile themeFile(QStringLiteral(":/LiveLoginDialog/postlive.qss"));
	if (themeFile.open(QIODevice::ReadOnly | QIODevice::Text))
		a.setStyleSheet(QString::fromUtf8(themeFile.readAll()));

	SetUnhandledExceptionFilter(ExceptionFilter);

	// 加载设置和用户列表
	thatboy::utils::loadConfig();
	thatboy::utils::loadUserData();

	// CI / 冒烟测试：POSTLIVE_SMOKE_TEST=1 在登录窗口自动退出；=platform 无头
	// 构造主窗口（设备枚举、预览控件、窗口列表）后退出；=login 渲染登录窗口
	// 并把画面保存到 POSTLIVE_SNAPSHOT 指定的路径（无头视觉检查）。
	const QByteArray smokeMode = qEnvironmentVariable("POSTLIVE_SMOKE_TEST").toUtf8();
	if (!smokeMode.isEmpty())
	{
		if (smokeMode == "platform")
		{
			LivePlatform w;
			w.show();
			QTimer::singleShot(1500, &a, [&]
				{
					const QString snapshot = qEnvironmentVariable("POSTLIVE_SNAPSHOT");
					if (!snapshot.isEmpty())
						w.grab().save(snapshot);
					QCoreApplication::quit();
				});
			a.exec();
		}
		else if (smokeMode == "login")
		{
			LiveLoginDialog d;
			d.show();
			QTimer::singleShot(500, &a, [&]
				{
					const QString snapshot = qEnvironmentVariable("POSTLIVE_SNAPSHOT");
					if (!snapshot.isEmpty())
						d.grab().save(snapshot);
					QCoreApplication::quit();
				});
			a.exec();
		}
		else
			QTimer::singleShot(500, &a, &QCoreApplication::quit);
	}

#ifndef _DEBUG
	try
	{
#endif
		if (smokeMode == "platform" || smokeMode == "login")
			return 0;

		LiveLoginDialog d;
		if (d.exec() == QDialog::Accepted)
		{
			// Animated splash covering LivePlatform construction (device
			// enumeration and SDK init can take a moment).
			QSplashScreen splash;
			QMovie splashMovie(QStringLiteral(":/main/res/splash.gif"));
			if (splashMovie.isValid())
			{
				QObject::connect(&splashMovie, &QMovie::frameChanged, &splash, [&]
					{
						splash.setPixmap(splashMovie.currentPixmap());
					});
				splashMovie.start();
				splash.show();
				splash.showMessage(QObject::tr("Loading..."), Qt::AlignCenter, Qt::white);
			}

			LivePlatform w;
			splash.finish(&w);
			w.show();
			a.exec();
		}
		bool saved = thatboy::utils::saveUserData();
		saved = thatboy::utils::saveConfig() && saved;
		if (!saved)
			qCCritical(lcApp) << "Failed to save config or user data; check write access to the program directory.";
		return 0;
#ifndef _DEBUG
	}
	catch (...)
	{
		qCCritical(lcApp) << "Unexpected exception; attempting to save data before exiting.";
		bool saved = false;
		try {
			saved = thatboy::utils::saveUserData();
			saved = thatboy::utils::saveConfig() && saved;
		}
		catch (...)
		{
			qCCritical(lcApp) << "Unexpected exception; saving data failed.";
			return -2;
		}
		if (!saved)
		{
			qCCritical(lcApp) << "Unexpected exception; data could not be saved.";
			return -2;
		}
		qCCritical(lcApp) << "Unexpected exception; data has been saved.";
		return -1;
	}
#endif
}