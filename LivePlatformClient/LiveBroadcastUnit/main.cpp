#include "LiveBroadcastUnit.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    LiveBroadcastUnit w;
    w.show();
    return a.exec();
}
