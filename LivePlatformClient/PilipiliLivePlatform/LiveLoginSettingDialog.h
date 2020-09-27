#pragma once

#include "DragableDialog.h"
#include "ui_LiveLoginSettingDialog.h"

class LiveLoginSettingDialog : public DragableDialog
{
	Q_OBJECT

public:
	LiveLoginSettingDialog(QWidget *parent = Q_NULLPTR);
	~LiveLoginSettingDialog();

private:
	Ui::LiveLoginSettingDialog ui;
};
