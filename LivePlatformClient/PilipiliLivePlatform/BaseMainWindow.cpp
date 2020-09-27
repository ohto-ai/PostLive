#include "BaseMainWindow.h"
#include <QMouseEvent>

BaseMainWindow::BaseMainWindow(QWidget* parent, Qt::WindowFlags f)
	: QMainWindow(parent, f)
{
}

BaseMainWindow::~BaseMainWindow()
{
}

void BaseMainWindow::showEvent(QShowEvent*)
{
    emit showed();
}

void BaseMainWindow::hideEvent(QHideEvent*)
{
    emit hided();
}

void BaseMainWindow::closeEvent(QCloseEvent*)
{
    emit closed();
}

void BaseMainWindow::moveEvent(QMoveEvent*)
{
    emit moved();
}

void BaseMainWindow::resizeEvent(QResizeEvent*)
{
    emit resized();
}

void BaseMainWindow::mousePressEvent(QMouseEvent*e)
{
    if (e->button() & Qt::LeftButton) {
        emit leftClicked();
        emit clicked();
    }
    else if (e->button() & Qt::RightButton)
    {
        emit rightClicked();
        emit clicked();
    }
    else if (e->button() & Qt::MiddleButton)
    {
        emit middleClicked();
        emit clicked();
    }
    QWidget::mousePressEvent(e);
}