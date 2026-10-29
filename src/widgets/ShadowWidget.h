#pragma once

#include <QEvent>
#include <QPointer>
#include "BaseWidget.h"
#include "BaseDialog.h"
#include "BaseMainWindow.h"

class ShadowWidget : public BaseWidget
{
	Q_OBJECT

public:
	ShadowWidget(QWidget* parent);
	~ShadowWidget();

	void show();
	void execute(BaseWidget* w);
	void execute(BaseDialog* d);
	void execute(BaseMainWindow* m);

protected:
	bool eventFilter(QObject* watched, QEvent* event) override;

private:
	bool active{ false };
	QPointer<QWidget> shadowed;
};
