#include "TouchedLabel.h"

TouchedLabel::TouchedLabel(QWidget *parent)
	: QLabel(parent)
{
}

TouchedLabel::~TouchedLabel()
{
}

void TouchedLabel::mousePressEvent(QMouseEvent* e)
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
    QLabel::mousePressEvent(e);
}