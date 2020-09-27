#include "BaseMainWindow.h"
#include <QMouseEvent>

BaseMainWindow::BaseMainWindow(QWidget* parent, Qt::WindowFlags f)
	: QMainWindow(parent, f)
{
}

BaseMainWindow::~BaseMainWindow()
{
}

void BaseMainWindow::showEvent(QShowEvent* e)
{
    emit showed();
    QMainWindow::showEvent(e);
}

void BaseMainWindow::hideEvent(QHideEvent* e)
{
    emit hided();
    QMainWindow::hideEvent(e);
}

void BaseMainWindow::closeEvent(QCloseEvent* e)
{
    emit closed();
    QMainWindow::closeEvent(e);
}

void BaseMainWindow::moveEvent(QMoveEvent* e)
{
    emit moved(e->oldPos(), e->pos());
    QMainWindow::moveEvent(e);
}

void BaseMainWindow::resizeEvent(QResizeEvent* e)
{
    emit resized(e->oldSize(), e->size());
    QMainWindow::resizeEvent(e);
}

void BaseMainWindow::mousePressEvent(QMouseEvent* e)
{
    if (e->button() & Qt::LeftButton) {
        emit leftClicked();
    }
    if (e->button() & Qt::RightButton)
    {
        emit rightClicked();
    }
    if (e->button() & Qt::MiddleButton)
    {
        emit middleClicked();
    }
    emit clicked();
    QMainWindow::mousePressEvent(e);
}

void BaseMainWindow::focusInEvent(QFocusEvent* e)
{
    emit focusIn();
    QMainWindow::focusInEvent(e);
}

void BaseMainWindow::focusOutEvent(QFocusEvent* e)
{
    emit focusOut();
    QMainWindow::focusOutEvent(e);
}
