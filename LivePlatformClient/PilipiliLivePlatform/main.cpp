#pragma execution_character_set("utf-8")
#include "LivePlatform.h"
#include "LiveLoginDialog.h"
#include <QtWidgets/QApplication>
#include <DbgHelp.h>

#pragma comment(lib, "Dbghelp.lib")

int GenerateDump(EXCEPTION_POINTERS* pExceptionPointers)
{
    // 创建Dump文件
    HANDLE hDumpFile = CreateFile(L"LivePlatform.dmp", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
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
	SetUnhandledExceptionFilter(ExceptionFilter);

	// 加载设置和用户列表
	thatboy::utils::loadConfig();
	thatboy::utils::loadUserData();

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
		thatboy::utils::saveUserData();
		thatboy::utils::saveConfig();
		return 0;
#ifndef _DEBUG
	}
	catch (...)
	{
		try {
			thatboy::utils::saveUserData();
			thatboy::utils::saveConfig();
		}
		catch (...)
		{
			QMessageBox::warning(nullptr, "错误", "未预料的错误，数据保存失败.");
			return -2;
		}
		QMessageBox::warning(nullptr, "错误", "未预料的错误，数据已保存.");
		return -1;
	}
#endif
}