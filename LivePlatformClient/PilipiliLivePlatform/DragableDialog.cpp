#include "DragableDialog.h"
#include <QMouseEvent>
#include <QApplication>

DragableDialog::DragableDialog(QWidget *parent)
	: QDialog(parent)
{
}

DragableDialog::~DragableDialog()
{
}

void DragableDialog::mousePressEvent(QMouseEvent* event)
{
    if (Qt::LeftButton == event->button() && !isMaximized())
    {
        QApplication::setOverrideCursor(QCursor(Qt::OpenHandCursor));
        mouseLastPos = event->globalPos();
        event->ignore();
    }
    mouseDown = true;
}

void DragableDialog::mouseReleaseEvent(QMouseEvent* event)
{
    QApplication::restoreOverrideCursor();
    event->ignore();
    mouseDown = false;
}

void DragableDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (mouseDown && !isMaximized() && (event->buttons() & Qt::LeftButton))
    {
        move(event->globalPos() + pos() - mouseLastPos);
        mouseLastPos = event->globalPos();
    }
}
