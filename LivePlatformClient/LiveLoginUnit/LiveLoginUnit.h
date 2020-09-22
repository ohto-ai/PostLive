#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_LiveLoginUnit.h"

class LiveLoginUnit : public QMainWindow
{
    Q_OBJECT

public:
    LiveLoginUnit(QWidget *parent = Q_NULLPTR);

private:
    Ui::LiveLoginUnitClass ui;
};
