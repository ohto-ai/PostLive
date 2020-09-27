#include "LiveLoginSettingDialog.h"

LiveLoginSettingDialog::LiveLoginSettingDialog(QWidget *parent)
	: DragableDialog(parent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_TranslucentBackground);
	connect(ui.closeToolButton, &QToolButton::clicked, this, &LiveLoginSettingDialog::close);
	connect(ui.minimizeToolButton, &QToolButton::clicked, this, &LiveLoginSettingDialog::showMinimized);
}

LiveLoginSettingDialog::~LiveLoginSettingDialog()
{
}
