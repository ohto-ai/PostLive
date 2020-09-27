#include "LivePlatform.h"

LivePlatform::LivePlatform(QWidget *parent)
    : DragableMainWindow(parent)
{
    ui.setupUi(this);
	setAttribute(Qt::WA_TranslucentBackground);
	connect(ui.closeToolButton, &QToolButton::clicked, this, &LivePlatform::close);
	connect(ui.minimizeToolButton, &QToolButton::clicked, this, &LivePlatform::showMinimized);
}
