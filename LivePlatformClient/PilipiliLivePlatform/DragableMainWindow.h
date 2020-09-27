#pragma once

#include "BaseMainWindow.h"

class DragableMainWindow : public BaseMainWindow
{
protected:
    DragableMainWindow(QWidget* parent, Qt::WindowFlags f = 0);
public:
    virtual ~DragableMainWindow();

protected:
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
protected:
    bool mouseDown{ false };
    QPoint mouseLastPos;
};
