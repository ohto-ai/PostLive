#include "ShadowWidget.h"

ShadowWidget::ShadowWidget(QWidget* parent)
	: QWidget(parent, Qt::FramelessWindowHint | Qt::Window)
{
	setPalette(QPalette(Qt::black));
	setWindowOpacity(0.6);
}

void ShadowWidget::show()
{
	setGeometry(dynamic_cast<QWidget*>(parent())->frameGeometry());
	QWidget::show();
}

ShadowWidget::~ShadowWidget()
{
}

void ShadowWidget::showShadowWindow(QWidget* w)
{
	show();
	w->setWindowModality(Qt::ApplicationModal);
	w->show();
}

void ShadowWidget::mousePressEvent(QMouseEvent* e)
{
	close();
}
