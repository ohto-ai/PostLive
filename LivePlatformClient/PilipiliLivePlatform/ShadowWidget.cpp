#include "ShadowWidget.h"

ShadowWidget::ShadowWidget(QWidget *parent)
	: BaseWidget(parent, Qt::FramelessWindowHint | Qt::Window)
{
	setPalette(QPalette(Qt::black));
	setWindowOpacity(0.6);
	connect(this, &BaseWidget::clicked, this, &BaseWidget::close);
}

ShadowWidget::~ShadowWidget()
{
}

void ShadowWidget::show()
{
	auto parentWidget = dynamic_cast<QWidget*>(parent());
	if (parentWidget != nullptr)
	{
		setGeometry(parentWidget->frameGeometry());
		QWidget::show();
	}
}

void ShadowWidget::execute(BaseWidget* widget)
{
	connect(widget, &BaseWidget::hided, this, &ShadowWidget::close);
	show();
	widget->setWindowModality(Qt::WindowModal);
	widget->show();
}
void ShadowWidget::execute(BaseDialog* dialog)
{
	connect(dialog, &BaseDialog::hided, this, &ShadowWidget::close);
	show();
	dialog->setWindowModality(Qt::WindowModal);
	dialog->show();
}
void ShadowWidget::execute(BaseMainWindow* mainwindow)
{
	connect(mainwindow, &BaseMainWindow::hided, this, &ShadowWidget::close);
	show();
	mainwindow->setWindowModality(Qt::WindowModal);
	mainwindow->show();
}
