#include "ShadowWindowDemo.h"
#include "ShadowWidget.h"
#include "TestWindow.h"
#include <QMessageBox>

ShadowWindowDemo::ShadowWindowDemo(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    auto& shadow = *new ShadowWidget(this);
    auto& testWindow = *new TestWindow(this);
        
    connect(ui.callShadowWidgetpushButton, &QPushButton::clicked, 
        [&]
        {
            shadow.showShadowWindow(&testWindow);
        });
}
