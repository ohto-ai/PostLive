#include "BaseWidget.h"
#include <QMouseEvent>
#include <QResizeEvent>

BaseWidget::BaseWidget(QWidget* parent, Qt::WindowFlags f)
	: QWidget(parent, f)
{
}

BaseWidget::~BaseWidget()
{
}

void BaseWidget::showEvent(QShowEvent*e)
{
    emit showed();
    QWidget::showEvent(e);
}

void BaseWidget::hideEvent(QHideEvent*e)
{
    emit hided();
    QWidget::hideEvent(e);
}

void BaseWidget::closeEvent(QCloseEvent*e)
{
    emit closed();
    QWidget::closeEvent(e);
}

void BaseWidget::moveEvent(QMoveEvent*e)
{
    emit moved(e->oldPos(), e->pos());
    QWidget::moveEvent(e);
}

void BaseWidget::resizeEvent(QResizeEvent*e)
{
    emit resized(e->oldSize(), e->size());
    QWidget::resizeEvent(e);
}

void BaseWidget::mousePressEvent(QMouseEvent*e)
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
    QWidget::mousePressEvent(e);
}

void BaseWidget::focusInEvent(QFocusEvent*e)
{
    emit focusIn();
    QWidget::focusInEvent(e);
}

void BaseWidget::focusOutEvent(QFocusEvent*e)
{
    emit focusOut();
    QWidget::focusOutEvent(e);
}
