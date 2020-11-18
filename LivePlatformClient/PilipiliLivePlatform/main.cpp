#pragma execution_character_set("utf-8")
#include "LivePlatform.h"
#include "LiveLoginDialog.h"
#include <QtWidgets/QApplication>

#define LOCAL_SERVER_TEST

int main(int argc, char* argv[])
{
	QApplication a(argc, argv);

#ifdef LOCAL_SERVER_TEST
	QProcess process;
	process.start("InnerServer.exe");
#endif

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