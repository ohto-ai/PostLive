#include "LiveLoginSettingDialog.h"

LiveLoginSettingDialog::LiveLoginSettingDialog(QWidget* parent)
	: DragableDialog(parent)
{
	ui.setupUi(this);

	thatboy::storage::config["widget_info"]["login_setting"].get<thatboy::WidgetConfigInfo>().config(*this); 
	connect(this, &BaseDialog::closed, [&]
		{
			thatboy::storage::config["widget_info"]["login_setting"] = thatboy::WidgetConfigInfo(*this);
		});

	connect(ui.closeToolButton, &QToolButton::clicked, this, &LiveLoginSettingDialog::close);
	connect(ui.minimizeToolButton, &QToolButton::clicked, this, &LiveLoginSettingDialog::showMinimized);
}

LiveLoginSettingDialog::~LiveLoginSettingDialog()
{
}