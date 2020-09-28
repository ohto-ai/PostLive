#include "DragableMainWindow.h"
#include <QMouseEvent>
#include <QApplication>

DragableMainWindow::DragableMainWindow(QWidget* parent, Qt::WindowFlags f)
	: BaseMainWindow(parent, f | Qt::FramelessWindowHint)
{
}

DragableMainWindow::~DragableMainWindow()
{
}

void DragableMainWindow::mousePressEvent(QMouseEvent* event)
{
	if (Qt::LeftButton == event->button() && !isMaximized())
	{
		QApplication::setOverrideCursor(QCursor(Qt::OpenHandCursor));
		mouseLastPos = event->globalPos();
		event->ignore();
	}
	mouseDown = true;
}

void DragableMainWindow::mouseReleaseEvent(QMouseEvent* event)
{
	QApplication::restoreOverrideCursor();
	event->ignore();
	mouseDown = false;
}

void DragableMainWindow::mouseMoveEvent(QMouseEvent* event)
{
	if (mouseDown && !isMaximized() && (event->buttons() & Qt::LeftButton))
	{
		move(event->globalPos() + pos() - mouseLastPos);
		mouseLastPos = event->globalPos();
	}
}