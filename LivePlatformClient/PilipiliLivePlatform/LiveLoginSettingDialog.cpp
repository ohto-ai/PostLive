#include "LiveLoginSettingDialog.h"

LiveLoginSettingDialog::LiveLoginSettingDialog(QWidget *parent)
	: DragableDialog(parent)
{
	ui.setupUi(this);
	connect(ui.closeToolButton, &QToolButton::clicked, this, &QDialog::close);
	connect(ui.minimizeToolButton, &QToolButton::clicked, this, &QDialog::showMinimized);
}

LiveLoginSettingDialog::~LiveLoginSettingDialog()
{
}
