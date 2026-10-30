#include "LiveLoginSettingDialog.h"

#include "../core/Logging.h"
#include "../media/FFmpegInputDevice.h"

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
	ui.localHlsCheckBox->setChecked(config["stream"].value("local_hls", false));

	ui.streamSizeComboBox->addItems({
		"640x480", "800x600", "1024x768", "1280x720", "1280x800", "1400x1050", "1440x900", "1600x1200", "1920x1080", "1920x1200"
		});
	ui.streamBitrateComboBox->addItems({ "Auto", "1000", "2500", "4000", "6000" });
	ui.streamFpsComboBox->addItems({ "Auto", "15", "24", "30", "60" });
	ui.streamEncoderComboBox->addItems({ "Auto", "libx264", "h264_nvenc", "h264_amf", "h264_qsv", "libopenh264" });

	auto restoreStreamCombo = [&](QComboBox* combo, const char* key, const char* fallback)
	{
		combo->setCurrentText(QString::fromStdString(
			config["stream"].value(key, std::string(fallback))));
	};
	restoreStreamCombo(ui.streamSizeComboBox, "size", "1280x720");
	restoreStreamCombo(ui.streamBitrateComboBox, "bitrate", "Auto");
	restoreStreamCombo(ui.streamFpsComboBox, "fps", "Auto");
	restoreStreamCombo(ui.streamEncoderComboBox, "encoder", "Auto");
	ui.streamServerLineEdit->setText(QString::fromStdString(
		config["stream"].value("server", std::string("rtmp://rtmp.ohtoai.top"))));

	FFmpegInputDevice::updateInputDeviceList();
	ui.streamMicComboBox->addItem("virtual-audio-capturer");
	for (const FFmpegInputDevice* microphone : FFmpegInputDevice::filterAudioDevices())
		ui.streamMicComboBox->addItem(QString::fromStdString(microphone->device_name));
	{
		const QString saved = QString::fromStdString(
			config["stream"].value("microphone", std::string("virtual-audio-capturer")));
		const int index = ui.streamMicComboBox->findText(saved);
		if (index >= 0)
			ui.streamMicComboBox->setCurrentIndex(index);
	}

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
	connect(ui.streamServerLineEdit, &QLineEdit::textEdited, this, [this]
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

	const QString streamServer = ui.streamServerLineEdit->text().trimmed();
	const QUrl streamUrl(streamServer);
	const QString streamScheme = streamUrl.scheme().toLower();
	if (streamServer.isEmpty() || !streamUrl.isValid() || streamUrl.host().isEmpty()
		|| (streamScheme != "rtmp" && streamScheme != "rtmps"))
	{
		qCWarning(lcCore) << "settings rejected: invalid stream server URL" << streamServer;
		showFieldError(ui.streamServerLineEdit, "推流服务器地址无效，示例：rtmp://server:1935");
		return;
	}

	auto& config = thatboy::storage::config;
	config["auth_server"] = server.toStdString();
	config["website"] = website.toStdString();
	config["stream"]["watermark"] = ui.watermarkCheckBox->isChecked();
	config["stream"]["local_hls"] = ui.localHlsCheckBox->isChecked();
	config["stream"]["server"] = streamServer.toStdString();
	config["stream"]["size"] = ui.streamSizeComboBox->currentText().toStdString();
	config["stream"]["bitrate"] = ui.streamBitrateComboBox->currentText().toStdString();
	config["stream"]["fps"] = ui.streamFpsComboBox->currentText().toStdString();
	config["stream"]["encoder"] = ui.streamEncoderComboBox->currentText().toStdString();
	config["stream"]["microphone"] = ui.streamMicComboBox->currentText().toStdString();
	thatboy::storage::websiteUrl = website;
	qCInfo(lcCore) << "settings saved: auth_server" << server << "website" << website
		<< "stream server" << streamServer << "size" << QString::fromStdString(config["stream"]["size"].get<std::string>());
	close();
}
