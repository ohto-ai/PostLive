#include "BaseWidget.h"
#include <QMouseEvent>

BaseWidget::BaseWidget(QWidget* parent, Qt::WindowFlags f)
	: QWidget(parent, f)
{
}

BaseWidget::~BaseWidget()
{
}

void BaseWidget::showEvent(QShowEvent*)
{
    emit showed();
}

void BaseWidget::hideEvent(QHideEvent*)
{
    emit hided();
}

void BaseWidget::closeEvent(QCloseEvent*)
{
    emit closed();
}

void BaseWidget::moveEvent(QMoveEvent*)
{
    emit moved();
}

void BaseWidget::resizeEvent(QResizeEvent*)
{
    emit resized();
}

void BaseWidget::mousePressEvent(QMouseEvent*e)
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