#include "TouchedLineEdit.h"
#include <QMouseEvent>

TouchedLineEdit::TouchedLineEdit(QWidget* parent)
	: QLineEdit(parent)
{
}

TouchedLineEdit::~TouchedLineEdit()
{
}

void TouchedLineEdit::mousePressEvent(QMouseEvent* e)
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
	QLineEdit::mousePressEvent(e);
}

void TouchedLineEdit::focusInEvent(QFocusEvent* e)
{
	emit focusIn();
	QLineEdit::focusInEvent(e);
}

void TouchedLineEdit::focusOutEvent(QFocusEvent* e)
{
	emit focusOut();
	QLineEdit::focusOutEvent(e);
}