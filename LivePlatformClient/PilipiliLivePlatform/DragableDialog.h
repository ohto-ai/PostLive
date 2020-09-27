#pragma once

#include <QDialog>

class DragableDialog : public QDialog
{
protected:
	DragableDialog(QWidget *parent);
public:
	virtual ~DragableDialog();

protected:
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
protected:
    bool mouseDown{ false };
    QPoint mouseLastPos;
};
