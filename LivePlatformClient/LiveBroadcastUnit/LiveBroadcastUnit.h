#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_LiveBroadcastUnit.h"

class LiveBroadcastUnit : public QMainWindow
{
    Q_OBJECT

public:
    LiveBroadcastUnit(QWidget *parent = Q_NULLPTR);

private:
    Ui::LiveBroadcastUnitClass ui;
};
