#pragma once

#include <QProcess>
#include "utils.h"
#include "DragableMainWindow.h"
#include "ui_LivePlatform.h"

class LivePlatform : public DragableMainWindow
{
    Q_OBJECT

public:
    LivePlatform(QWidget *parent = Q_NULLPTR);
protected:
    QProcess ffmpegProcess{ this };
private:
    Ui::LivePlatformClass ui;
};
