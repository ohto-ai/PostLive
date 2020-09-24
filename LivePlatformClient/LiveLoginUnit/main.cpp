#include "LiveLoginUnit.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 加载设置和用户列表
    thatboy::utils::loadConfig();
    thatboy::utils::loadUsers();

    LiveLoginUnit w;
    w.show();
    return a.exec();
}
