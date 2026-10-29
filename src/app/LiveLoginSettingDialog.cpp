#include "LiveLoginSettingDialog.h"

#include "../core/Logging.h"

#include <QCheckBox>
#include <QLineEdit>
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

	// Editing a field clears its error highlight and the inline message.
	connect(ui.accountServerLineEdit, &QLineEdit::textEdited, this, [this]
		{
			showFieldError(nullptr, QString());
		});
	connect(ui.websiteLineEdit, &QLineEdit::textEdited, this, [this]
		{
			showFieldError(nullptr, QString());
		});
}

LiveLoginSettingDialog::~LiveLoginSettingDialog()
{
}

void LiveLoginSettingDialog::showFieldError(QLineEdit* field, const QString& message)
{
	if (errorField)
		errorField->setStyleSheet({});
	errorField = field;
	if (errorField)
		errorField->setStyleSheet(QStringLiteral("QLineEdit { border-color: #E74C3C; }"));
	ui.messageLabel->setText(message);
}

void LiveLoginSettingDialog::save()
{
	const QString server = ui.accountServerLineEdit->text().trimmed();
	const QUrl serverUrl(server);
	const QString serverScheme = serverUrl.scheme().toLower();
	if (server.isEmpty() || !serverUrl.isValid() || serverUrl.host().isEmpty()
		|| (serverScheme != "http" && serverScheme != "https"))
	{
		qCWarning(lcCore) << "settings rejected: invalid auth server URL" << server;
		showFieldError(ui.accountServerLineEdit,
			"认证服务器地址无效，示例：http://localhost:8877");
		return;
	}
	if (!serverUrl.path().isEmpty() && serverUrl.path() != "/")
	{
		qCWarning(lcCore) << "settings rejected: auth server URL carries a path" << server;
		showFieldError(ui.accountServerLineEdit,
			"认证服务器只填写协议、主机和端口，不要包含路径。");
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
			qCWarning(lcCore) << "settings rejected: invalid website URL" << website;
			showFieldError(ui.websiteLineEdit, "官网地址无效，示例：https://example.com");
			return;
		}
	}

	auto& config = thatboy::storage::config;
	config["auth_server"] = server.toStdString();
	config["website"] = website.toStdString();
	config["stream"]["watermark"] = ui.watermarkCheckBox->isChecked();
	thatboy::storage::websiteUrl = website;
	qCInfo(lcCore) << "settings saved: auth_server" << server << "website" << website;
	close();
}
