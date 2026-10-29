#pragma execution_character_set("utf-8")
#include "LivePlatform.h"
#include "LiveLoginDialog.h"
#include <QtWidgets/QApplication>
#include <QDir>
#include <QNetworkProxyFactory>
#include <QTimer>
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
		QMessageBox::critical(nullptr, "错误", "发生了未处理的异常.");
		return EXCEPTION_CONTINUE_SEARCH;
	}
	else {
		QMessageBox::critical(nullptr, "错误", "发生了未处理的异常，程序即将退出. 将生成转储文件...");
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

	SetUnhandledExceptionFilter(ExceptionFilter);

	// 加载设置和用户列表
	thatboy::utils::loadConfig();
	thatboy::utils::loadUserData();

	// CI / 冒烟测试：启动后自动退出，便于验证配置与用户数据保存。
	if (qEnvironmentVariableIsSet("POSTLIVE_SMOKE_TEST"))
		QTimer::singleShot(500, &a, &QCoreApplication::quit);

#ifndef _DEBUG
	try
	{
#endif
		LiveLoginDialog d;
		if (d.exec() == QDialog::Accepted)
		{
			LivePlatform w;
			w.show();
			a.exec();
		}
		bool saved = thatboy::utils::saveUserData();
		saved = thatboy::utils::saveConfig() && saved;
		if (!saved)
			QMessageBox::warning(nullptr, "错误", "配置或用户数据保存失败，请检查程序目录的写入权限.");
		return 0;
#ifndef _DEBUG
	}
	catch (...)
	{
		bool saved = false;
		try {
			saved = thatboy::utils::saveUserData();
			saved = thatboy::utils::saveConfig() && saved;
		}
		catch (...)
		{
			QMessageBox::warning(nullptr, "错误", "未预料的错误，数据保存失败.");
			return -2;
		}
		if (!saved)
		{
			QMessageBox::warning(nullptr, "错误", "未预料的错误，数据保存失败.");
			return -2;
		}
		QMessageBox::warning(nullptr, "错误", "未预料的错误，数据已保存.");
		return -1;
	}
#endif
}