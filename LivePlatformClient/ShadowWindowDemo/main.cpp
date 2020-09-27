#include "ShadowWindowDemo.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ShadowWindowDemo w;
    w.show();
    return a.exec();
}
