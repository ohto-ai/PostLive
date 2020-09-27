#include "BaseDialog.h"
#include <QMouseEvent>

BaseDialog::BaseDialog(QWidget* parent, Qt::WindowFlags f)
	: QDialog(parent, f)
{
}

BaseDialog::~BaseDialog()
{
}

void BaseDialog::showEvent(QShowEvent*)
{
    emit showed();
}

void BaseDialog::hideEvent(QHideEvent*)
{
    emit hided();
}

void BaseDialog::closeEvent(QCloseEvent*)
{
    emit closed();
}

void BaseDialog::moveEvent(QMoveEvent*)
{
    emit moved();
}

void BaseDialog::resizeEvent(QResizeEvent*)
{
    emit resized();
}

void BaseDialog::mousePressEvent(QMouseEvent*e)
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