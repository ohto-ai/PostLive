#pragma once

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
};
