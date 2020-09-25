#pragma execution_character_set("utf-8")
#include "LivePlatform.h"
#include "LiveLoginDialog.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    // 加载设置和用户列表
    thatboy::utils::loadConfig();
    thatboy::utils::loadUserData();

    LiveLoginDialog d;
    d.exec();
    QMessageBox::information(nullptr, "Tip","后面还没做");
    LivePlatform w;
    w.show();
    return a.exec();
}
