#pragma once

#include <QtWidgets/QMainWindow>
#include <QToolButton>
#include <QUrl>
#include <QStyle>
#include <QMouseEvent>
#include "utils.h"
#include "ui_LiveLoginUnit.h"

class LiveLoginUnit : public QMainWindow
{
    Q_OBJECT

public:
    LiveLoginUnit(QWidget *parent = Q_NULLPTR);

    bool mouseIsPressed{ false };
    QPoint pLast;
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;

protected:

private:
    Ui::LiveLoginUnitClass ui;
};
