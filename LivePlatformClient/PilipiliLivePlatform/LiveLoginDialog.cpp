#pragma execution_character_set("utf-8")

#include "LiveLoginDialog.h"

LiveLoginDialog::LiveLoginDialog(QWidget* parent)
	: DragableDialog(parent)
{
	ui.setupUi(this);
	applyConfig();
}

void LiveLoginDialog::login()
{
	using thatboy::storage::currentUser;
	using thatboy::storage::usersStorage;

	std::string account = ui.accountLineEdit->text().toStdString();

	nlohmann::json loginPackage;
	loginPackage["account"] = account;
	loginPackage["type"] = "login";

	if (ui.passwordLineEdit->passwordType() == PasswordEdit::CheatPasswordFilled)
	{
		loginPackage["token"] = ui.passwordLineEdit->md5();
		loginPackage["login"] = "token";
	}
	else
	{
		loginPackage["password"] = ui.passwordLineEdit->md5();
		loginPackage["login"] = "password";
	}
	// 发送
	auto res = thatboy::storage::accountVerifyClient.Post("/api", loginPackage.dump(), "application/json");

	if (!res)
		QMessageBox::warning(this, "Warning", "无法连接到服务器.");
	else
	{
		auto body{ nlohmann::json::parse(res->body) };
		if (!body["success"])
			QMessageBox::warning(this, "Warning", "登录失败." + body["info"].get<QString>());
		else
		{
			currentUser["account"] = account;
			currentUser["token"] = body["token"];
			usersStorage["users"][account]["token"] = body["token"];
			usersStorage["users"][account]["pwmask"] = ui.passwordLineEdit->password().length();
			usersStorage["users"][account]["remember_password"] = ui.rememberPasswordCheckBox->isChecked();

			loginPackage["type"] = "profile";
			loginPackage["profile"].push_back("avatar");
			loginPackage["profile"].push_back("gender");
			loginPackage["token"] = currentUser["token"];
			res = thatboy::storage::accountVerifyClient.Post("/api", loginPackage.dump(), "application/json");
			body = nlohmann::json::parse(res->body);

			usersStorage["users"][account]["avatar"] = body["profile"]["avatar"];
			usersStorage["users"][account]["gender"] = body["profile"]["gender"];

			accept();
		}
	}
}

void LiveLoginDialog::applyConfig()
{
	auto& loginConfig = thatboy::storage::config["login"];
	setGeometry(loginConfig["geometry"]);
	setAttribute(Qt::WA_TranslucentBackground);
	setWindowOpacity(0.9);

	connect(ui.registerPushButton, &QPushButton::clicked
		, std::bind(&QDesktopServices::openUrl, QUrl(thatboy::storage::RegisterUrl)));
	connect(ui.forgetPasswordPushButton, &QPushButton::clicked
		, std::bind(&QDesktopServices::openUrl, QUrl(thatboy::storage::ResetPasswordUrl)));
	connect(ui.loginPushButton, &QPushButton::clicked, this, &LiveLoginDialog::login);
	connect(ui.accountLineEdit, &QLineEdit::textChanged, this, &LiveLoginDialog::setAvatar);
	connect(ui.minimizeToolButton, &QToolButton::clicked, this, &QDialog::showMinimized);
	connect(ui.closeToolButton, &QToolButton::clicked, this, &QDialog::close);
	connect(ui.settingToolButton, &QToolButton::clicked
		, std::bind(static_cast<void(ShadowWidget::*)(BaseDialog*)>(&ShadowWidget::execute), &shadowWidget, &settingDialog));

	connect(&thatboy::storage::generalSignal, &GeneralSignal::avatarDownloaded, [=](QString user, bool succcess)
		{
			if (succcess && user == ui.accountLineEdit->text())
				setAvatar();
		});

	setAvatar();
	setAccountCompleter();
	setAccountPasswordAcceptableInputCheck();

	ui.autoLoginCheckBox->setChecked(loginConfig["auto_login"]);

	connect(ui.accountLineEdit, &QLineEdit::textChanged, [&] {
		ui.passwordLineEdit->clear();
		using thatboy::storage::usersStorage;
		auto account = ui.accountLineEdit->text().toStdString();
		if (usersStorage["users"].contains(account))
		{
			ui.rememberPasswordCheckBox->setChecked(usersStorage["users"][account]["remember_password"]);

			if (usersStorage["users"][account]["remember_password"])
				ui.passwordLineEdit->fillCheatPassword(QString("%1").arg("a"
					, usersStorage["users"][account]["pwmask"].get<int>(), QLatin1Char('a'))
					, usersStorage["users"][account]["token"]
				);
		}
		});
	ui.accountLineEdit->setText(loginConfig["current_user"]);

	connect(this, &DragableDialog::closed, [&]
		{
			auto& loginConfig = thatboy::storage::config["login"];
			loginConfig["current_user"] = ui.accountLineEdit->text();
			loginConfig["auto_login"] = ui.autoLoginCheckBox->isChecked();

			thatboy::storage::config["login"]["geometry"] = geometry();
		});
}

void LiveLoginDialog::setAvatar()
{
	static const QPixmap defaultAvatarGroup[3]
	{
		thatboy::storage::DefaultAvatarUrlGroup[0]
		, thatboy::storage::DefaultAvatarUrlGroup[1]
		, thatboy::storage::DefaultAvatarUrlGroup[2]
	};
	auto userName = ui.accountLineEdit->text().toStdString();
	QPixmap avatar;
	if (thatboy::storage::usersStorage["users"].contains(userName) && !thatboy::storage::userAvatarCache[userName].isNull())
		avatar = thatboy::utils::roundedPixmap(thatboy::storage::userAvatarCache[userName]);
	if (!avatar.isNull())
		ui.avatarLabel->setPixmap(avatar);
	else
		ui.avatarLabel->setPixmap(defaultAvatarGroup[userName.length() / 5 % 3]);
}

void LiveLoginDialog::setAccountCompleter()
{
	auto& completer = *new QCompleter(thatboy::storage::usersStorage["user_names"], this);
	completer.setFilterMode(Qt::MatchFlag::MatchContains);
	ui.accountLineEdit->setCompleter(&completer);
}

void LiveLoginDialog::setAccountPasswordAcceptableInputCheck()
{
	connect(ui.accountLineEdit, &QLineEdit::textChanged
		, this, &LiveLoginDialog::checkAccountPasswordAcceptableInput);
	connect(ui.passwordLineEdit, &QLineEdit::textChanged
		, this, &LiveLoginDialog::checkAccountPasswordAcceptableInput);
	ui.accountLineEdit->setValidator(new QRegExpValidator(
		QRegExp{ thatboy::storage::RegexAccount }, this));
	ui.passwordLineEdit->setRegExpValidator(QRegExp{ thatboy::storage::RegexPassword });
	checkAccountPasswordAcceptableInput();
}

bool LiveLoginDialog::checkAccountPasswordAcceptableInput()
{
	ui.loginPushButton->setEnabled(ui.accountLineEdit->hasAcceptableInput()
		&& ui.passwordLineEdit->hasAcceptableInput());
	return ui.loginPushButton->isEnabled();
}