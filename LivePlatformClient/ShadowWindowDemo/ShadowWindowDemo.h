#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_ShadowWindowDemo.h"

class ShadowWindowDemo : public QMainWindow
{
    Q_OBJECT

public:
    ShadowWindowDemo(QWidget *parent = Q_NULLPTR);

private:
    Ui::ShadowWindowDemoClass ui;
};
