#include "BaseDialog.h"
#include <QMouseEvent>

BaseDialog::BaseDialog(QWidget* parent, Qt::WindowFlags f)
	: QDialog(parent, f)
{
}

BaseDialog::~BaseDialog()
{
}

void BaseDialog::showEvent(QShowEvent* e)
{
	emit showed();
	QDialog::showEvent(e);
}

void BaseDialog::hideEvent(QHideEvent* e)
{
	emit hided();
	QDialog::hideEvent(e);
}

void BaseDialog::closeEvent(QCloseEvent* e)
{
	emit closed();
	QDialog::closeEvent(e);
}

void BaseDialog::moveEvent(QMoveEvent* e)
{
	emit moved(e->oldPos(), e->pos());
	QDialog::moveEvent(e);
}

void BaseDialog::resizeEvent(QResizeEvent* e)
{
	emit resized(e->oldSize(), e->size());
	QDialog::resizeEvent(e);
}

void BaseDialog::mousePressEvent(QMouseEvent* e)
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
	QDialog::mousePressEvent(e);
}

void BaseDialog::focusInEvent(QFocusEvent* e)
{
	emit focusIn();
	QDialog::focusInEvent(e);
}

void BaseDialog::focusOutEvent(QFocusEvent* e)
{
	emit focusOut();
	QDialog::focusOutEvent(e);
}