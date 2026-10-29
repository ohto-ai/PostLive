#include "ShadowWidget.h"

ShadowWidget::ShadowWidget(QWidget* parentWindow)
	: BaseWidget(parentWindow, Qt::FramelessWindowHint | Qt::Window)
{
	setPalette(QPalette(Qt::black));
	setWindowOpacity(0.6);
	setAttribute(Qt::WA_TranslucentBackground, false);
	connect(this, &BaseWidget::clicked, this, &BaseWidget::close);
	if (auto* parentWidget = qobject_cast<QWidget*>(parentWindow))
		parentWidget->installEventFilter(this);
}

ShadowWidget::~ShadowWidget()
{
}

void ShadowWidget::show()
{
	auto* parentWidget = qobject_cast<QWidget*>(parent());
	if (parentWidget != nullptr && parentWidget->isVisible() && !parentWidget->isMinimized())
	{
		setGeometry(parentWidget->frameGeometry());
		QWidget::show();
		if (shadowed)
			shadowed->raise();
	}
}

void ShadowWidget::execute(BaseWidget* widget)
{
	active = true;
	shadowed = widget;
	connect(widget, &BaseWidget::hided, this, [this] { hide(); });
	connect(widget, &BaseWidget::closed, this, [this]
		{
			active = false;
			shadowed = nullptr;
			close();
		});
	show();
	widget->setWindowModality(Qt::WindowModal);
	widget->show();
	widget->raise();
}

void ShadowWidget::execute(BaseDialog* dialog)
{
	active = true;
	shadowed = dialog;
	connect(dialog, &BaseDialog::hided, this, [this] { hide(); });
	connect(dialog, &BaseDialog::closed, this, [this]
		{
			active = false;
			shadowed = nullptr;
			close();
		});
	connect(dialog, &QDialog::finished, this, [this]
		{
			active = false;
			shadowed = nullptr;
			close();
		});
	show();
	dialog->setWindowModality(Qt::WindowModal);
	dialog->show();
	dialog->raise();
}

void ShadowWidget::execute(BaseMainWindow* mainwindow)
{
	active = true;
	shadowed = mainwindow;
	connect(mainwindow, &BaseMainWindow::hided, this, [this] { hide(); });
	connect(mainwindow, &BaseMainWindow::closed, this, [this]
		{
			active = false;
			shadowed = nullptr;
			close();
		});
	show();
	mainwindow->setWindowModality(Qt::WindowModal);
	mainwindow->show();
	mainwindow->raise();
}

bool ShadowWidget::eventFilter(QObject* watched, QEvent* event)
{
	if (watched == parent())
	{
		switch (event->type())
		{
		case QEvent::Hide:
		case QEvent::Show:
		case QEvent::WindowStateChange:
		{
			auto* parentWidget = qobject_cast<QWidget*>(parent());
			if (parentWidget == nullptr)
				break;

			if (parentWidget->isMinimized() || !parentWidget->isVisible())
			{
				hide();
			}
			else if (active)
			{
				setGeometry(parentWidget->frameGeometry());
				QWidget::show();
				if (shadowed)
					shadowed->raise();
			}
			break;
		}
		default:
			break;
		}
	}
	return BaseWidget::eventFilter(watched, event);
}
