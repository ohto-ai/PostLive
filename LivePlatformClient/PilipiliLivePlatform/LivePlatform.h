#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_LivePlatform.h"

class LivePlatform : public QMainWindow
{
    Q_OBJECT

public:
    LivePlatform(QWidget *parent = Q_NULLPTR);

private:
    Ui::LivePlatformClass ui;
};
