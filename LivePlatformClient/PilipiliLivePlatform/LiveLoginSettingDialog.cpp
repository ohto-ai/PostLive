#include "LiveLoginSettingDialog.h"

LiveLoginSettingDialog::LiveLoginSettingDialog(QWidget *parent)
	: DragableDialog(parent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_TranslucentBackground);
	setWindowOpacity(0.9);
	connect(ui.closeToolButton, &QToolButton::clicked, this, &QDialog::close);
}

LiveLoginSettingDialog::~LiveLoginSettingDialog()
{
}
