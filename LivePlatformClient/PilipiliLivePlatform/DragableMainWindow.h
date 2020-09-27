#pragma once

#include <QMainWindow>

class DragableMainWindow : public QMainWindow
{
protected:
    DragableMainWindow(QWidget* parent);
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
