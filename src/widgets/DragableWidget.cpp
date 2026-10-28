#include "DragableWidget.h"
#include <QMouseEvent>
#include <QApplication>

DragableWidget::DragableWidget(QWidget* parent, Qt::WindowFlags f)
	: BaseWidget(parent, f | Qt::FramelessWindowHint)
{
}

DragableWidget::~DragableWidget()
{
}

void DragableWidget::mousePressEvent(QMouseEvent* event)
{
	if (Qt::LeftButton == event->button() && !isMaximized())
	{
		QApplication::setOverrideCursor(QCursor(Qt::OpenHandCursor));
		mouseLastPos = event->globalPos();
		event->ignore();
	}
	mouseDown = true;
}

void DragableWidget::mouseReleaseEvent(QMouseEvent* event)
{
	QApplication::restoreOverrideCursor();
	event->ignore();
	mouseDown = false;
}

void DragableWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (mouseDown && !isMaximized() && (event->buttons() & Qt::LeftButton))
	{
		move(event->globalPos() + pos() - mouseLastPos);
		mouseLastPos = event->globalPos();
	}
}