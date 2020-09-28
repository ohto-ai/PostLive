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
	process.start("LiveServerTestDemo.exe");
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

			thatboy::utils::saveUserData();
			thatboy::utils::saveConfig();
			return a.exec();
		}
		else
		{
			thatboy::utils::saveUserData();
			thatboy::utils::saveConfig();
			return 0;
		}
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
			QMessageBox::warning(nullptr, "错误", "发生灾难性故障，数据无法保存.");
			return -2;
		}
		QMessageBox::warning(nullptr, "错误", "发生灾难性故障，数据已保存.");
		return -1;
	}
#endif
}