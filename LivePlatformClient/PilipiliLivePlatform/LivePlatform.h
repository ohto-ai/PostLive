#pragma once

#include "DragableMainWindow.h"
#include "ui_LivePlatform.h"

class LivePlatform : public DragableMainWindow
{
    Q_OBJECT

public:
    LivePlatform(QWidget *parent = Q_NULLPTR);

private:
    Ui::LivePlatformClass ui;
};
