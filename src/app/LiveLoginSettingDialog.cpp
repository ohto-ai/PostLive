#include "LiveLoginSettingDialog.h"

#include <QCheckBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QUrl>

LiveLoginSettingDialog::LiveLoginSettingDialog(QWidget* parent)
	: DragableDialog(parent)
{
	ui.setupUi(this);

	thatboy::storage::config["widget_info"]["login_setting"].get<thatboy::WidgetConfigInfo>().config(*this);
	connect(this, &BaseDialog::closed, [&]
		{
			thatboy::storage::config["widget_info"]["login_setting"] = thatboy::WidgetConfigInfo(*this);
		});

	const auto& config = thatboy::storage::config;
	ui.accountServerLineEdit->setText(QString::fromStdString(
		config.value("auth_server", std::string("http://localhost:8877"))));
	ui.websiteLineEdit->setText(QString::fromStdString(
		config.value("website", std::string("https://ohtoai.top"))));
	ui.watermarkCheckBox->setChecked(config["stream"].value("watermark", true));

	connect(ui.savePushButton, &QPushButton::clicked, this, &LiveLoginSettingDialog::save);
	connect(ui.closeToolButton, &QToolButton::clicked, this, &LiveLoginSettingDialog::close);
	connect(ui.minimizeToolButton, &QToolButton::clicked, this, &LiveLoginSettingDialog::showMinimized);
}

LiveLoginSettingDialog::~LiveLoginSettingDialog()
{
}

void LiveLoginSettingDialog::save()
{
	const QString server = ui.accountServerLineEdit->text().trimmed();
	const QUrl serverUrl(server);
	const QString serverScheme = serverUrl.scheme().toLower();
	if (server.isEmpty() || !serverUrl.isValid() || serverUrl.host().isEmpty()
		|| (serverScheme != "http" && serverScheme != "https"))
	{
		QMessageBox::warning(this, "错误", "认证服务器地址无效，示例：http://localhost:8877");
		return;
	}
	if (!serverUrl.path().isEmpty() && serverUrl.path() != "/")
	{
		QMessageBox::warning(this, "错误", "认证服务器只填写协议、主机和端口，不要包含路径。");
		return;
	}
	if (serverScheme == "https")
	{
		QMessageBox::warning(this, "错误", "当前构建未启用 HTTPS（OpenSSL），请使用 http:// 地址。");
		return;
	}

	const QString website = ui.websiteLineEdit->text().trimmed();
	if (!website.isEmpty())
	{
		const QUrl websiteUrl(website);
		const QString websiteScheme = websiteUrl.scheme().toLower();
		if (!websiteUrl.isValid() || websiteUrl.host().isEmpty()
			|| (websiteScheme != "http" && websiteScheme != "https"))
		{
			QMessageBox::warning(this, "错误", "官网地址无效，示例：https://example.com");
			return;
		}
	}

	auto& config = thatboy::storage::config;
	config["auth_server"] = server.toStdString();
	config["website"] = website.toStdString();
	config["stream"]["watermark"] = ui.watermarkCheckBox->isChecked();
	thatboy::storage::websiteUrl = website;
	close();
}
