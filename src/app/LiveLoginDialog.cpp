#pragma execution_character_set("utf-8")

#include "LiveLoginDialog.h"

#include <QApplication>
#include <QCheckBox>
#include <QNetworkRequest>

LiveLoginDialog::LiveLoginDialog(QWidget* parent)
	: DragableDialog(parent)
{
	ui.setupUi(this);
	applyConfig();
}

void LiveLoginDialog::login()
{
	if (loginInProgress)
		return;

	using thatboy::storage::currentUser;
	using thatboy::storage::usersStorage;

	const QString account = ui.accountLineEdit->text();
	if (account.isEmpty())
		return;
	const std::string accountStd = account.toStdString();

	nlohmann::json loginPackage;
	loginPackage["account"] = accountStd;
	loginPackage["type"] = "login";

	const bool tokenLogin = ui.passwordLineEdit->passwordType() == PasswordEdit::CheatPasswordFilled;
	if (tokenLogin)
	{
		loginPackage["token"] = ui.passwordLineEdit->md5().toStdString();
		loginPackage["login"] = "token";
	}
	else
	{
		loginPackage["password"] = ui.passwordLineEdit->md5().toStdString();
		loginPackage["login"] = "password";
	}

	const QUrl server(thatboy::utils::authServerUrl());
	const QString scheme = server.scheme().toLower();
	if (!server.isValid() || server.host().isEmpty()
		|| (scheme != "http" && scheme != "https"))
	{
		QMessageBox::warning(this, "Warning",
			"认证服务器地址无效，请在设置中填写 http(s)://host:port。");
		return;
	}

	QUrl endpoint = server;
	endpoint.setPath("/api");
	endpoint.setQuery(QString());
	endpoint.setFragment(QString());

	QNetworkRequest request(endpoint);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	request.setTransferTimeout(5000);

	loginInProgress = true;
	ui.loginPushButton->setEnabled(false);
	QApplication::setOverrideCursor(Qt::WaitCursor);

	auto* reply = networkManager.post(request, QByteArray::fromStdString(loginPackage.dump()));
	connect(reply, &QNetworkReply::finished, this,
		[this, reply, loginPackage, accountStd, tokenLogin]
		{
			handleLoginReply(reply, loginPackage, accountStd, tokenLogin);
		});
}

void LiveLoginDialog::handleLoginReply(QNetworkReply* reply, nlohmann::json loginPackage,
	std::string accountStd, bool tokenLogin)
{
	const QUrl endpoint = reply->url();
	const QByteArray data = reply->readAll();
	const auto networkError = reply->error();
	const QString networkErrorString = reply->errorString();
	const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	reply->deleteLater();

	using thatboy::storage::currentUser;
	using thatboy::storage::usersStorage;

	if (networkError != QNetworkReply::NoError && data.isEmpty())
	{
		finishLoginAttempt();
		QMessageBox::warning(this, "Warning",
			"无法连接到服务器: " + networkErrorString);
		return;
	}

	auto body = nlohmann::json::parse(data.toStdString(), nullptr, false);
	if (body.is_discarded() || !body.is_object())
	{
		finishLoginAttempt();
		if (networkError != QNetworkReply::NoError)
			QMessageBox::warning(this, "Warning",
				QString("服务器返回 HTTP %1: %2").arg(httpStatus).arg(networkErrorString));
		else
			QMessageBox::warning(this, "Warning", "服务器返回了无效数据.");
		return;
	}

	if (!body.contains("success") || !body["success"].is_boolean() || !body["success"].get<bool>())
	{
		finishLoginAttempt();
		const std::string info = (body.contains("info") && body["info"].is_string())
			? body["info"].get<std::string>() : std::string();
		QMessageBox::warning(this, "Warning", "登录失败. " + QString::fromStdString(info));

		if (tokenLogin)
		{
			// 保存的 token 已失效，清除记住密码与自动登录，避免下次启动重复失败。
			usersStorage["users"][accountStd]["remember_password"] = false;
			ui.rememberPasswordCheckBox->setChecked(false);
			ui.autoLoginCheckBox->setChecked(false);
			ui.passwordLineEdit->clear();
		}
		return;
	}

	if (!body.contains("token") || !body["token"].is_string())
	{
		finishLoginAttempt();
		QMessageBox::warning(this, "Warning", "服务器没有返回登录 token.");
		return;
	}

	currentUser["account"] = accountStd;
	currentUser["token"] = body["token"];
	usersStorage["users"][accountStd]["token"] = body["token"];
	usersStorage["users"][accountStd]["pwmask"] = ui.passwordLineEdit->password().length();
	usersStorage["users"][accountStd]["remember_password"] = ui.rememberPasswordCheckBox->isChecked();

	auto& names = usersStorage["user_names"];
	if (names.is_array())
	{
		bool found = false;
		for (const auto& name : names)
		{
			if (name.is_string() && name.get<std::string>() == accountStd)
			{
				found = true;
				break;
			}
		}
		if (!found)
			names.push_back(accountStd);
	}

	loginPackage["type"] = "profile";
	loginPackage["profile"].push_back("avatar");
	loginPackage["profile"].push_back("gender");
	loginPackage["token"] = currentUser["token"];

	QNetworkRequest request(endpoint);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	request.setTransferTimeout(5000);

	auto* profileReply = networkManager.post(request, QByteArray::fromStdString(loginPackage.dump()));
	connect(profileReply, &QNetworkReply::finished, this,
		[this, profileReply, accountStd]
		{
			handleProfileReply(profileReply, accountStd);
		});
}

void LiveLoginDialog::handleProfileReply(QNetworkReply* reply, const std::string& accountStd)
{
	const QByteArray data = reply->readAll();
	const auto networkError = reply->error();
	const QString networkErrorString = reply->errorString();
	const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	reply->deleteLater();

	if (networkError != QNetworkReply::NoError && data.isEmpty())
	{
		finishLoginAttempt();
		QMessageBox::warning(this, "Warning",
			"登录成功，但无法获取用户资料: " + networkErrorString);
		accept();
		return;
	}

	auto body = nlohmann::json::parse(data.toStdString(), nullptr, false);
	if (body.is_discarded() || !body.is_object()
		|| !body.contains("success") || !body["success"].is_boolean()
		|| !body["success"].get<bool>()
		|| !body.contains("profile") || !body["profile"].is_object())
	{
		finishLoginAttempt();
		if (networkError != QNetworkReply::NoError)
			QMessageBox::warning(this, "Warning",
				QString("登录成功，但获取用户资料失败（HTTP %1: %2）").arg(httpStatus).arg(networkErrorString));
		else
			QMessageBox::warning(this, "Warning", "登录成功，但获取用户资料失败.");
		accept();
		return;
	}

	auto& usersStorage = thatboy::storage::usersStorage;
	if (body["profile"].contains("avatar") && body["profile"]["avatar"].is_string())
		usersStorage["users"][accountStd]["avatar"] = body["profile"]["avatar"];
	if (body["profile"].contains("gender") && body["profile"]["gender"].is_string())
		usersStorage["users"][accountStd]["gender"] = body["profile"]["gender"];

	finishLoginAttempt();
	accept();
}

void LiveLoginDialog::finishLoginAttempt()
{
	loginInProgress = false;
	QApplication::restoreOverrideCursor();
	checkAccountPasswordAcceptableInput();
}

void LiveLoginDialog::applyConfig()
{
	auto& loginConfig = thatboy::storage::config["login"];

	thatboy::storage::config["widget_info"]["login"].get<thatboy::WidgetConfigInfo>().config(*this);
	connect(this, &BaseDialog::closed, [&]
		{
			thatboy::storage::config["widget_info"]["login"] = thatboy::WidgetConfigInfo(*this);
		});

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
	connect(ui.autoLoginCheckBox, &QCheckBox::toggled, this, [this](bool checked)
		{
			if (!checked)
				autoLoginTimer.stop();
		});
	connect(ui.accountLineEdit, &QLineEdit::textEdited, this, [this]
		{
			autoLoginTimer.stop();
		});
	connect(ui.passwordLineEdit, &QLineEdit::textEdited, this, [this]
		{
			autoLoginTimer.stop();
		});

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
			loginConfig["current_user"] = ui.accountLineEdit->text().toStdString();
			loginConfig["auto_login"] = ui.autoLoginCheckBox->isChecked();
		});
	connect(this, &QDialog::accepted, [&]
		{
			auto& loginConfig = thatboy::storage::config["login"];
			loginConfig["current_user"] = ui.accountLineEdit->text().toStdString();
			loginConfig["auto_login"] = ui.autoLoginCheckBox->isChecked();
		});

	// 自动登录；勾选框取消或用户编辑账号/密码会取消本次自动登录。
	if (ui.autoLoginCheckBox->isChecked())
	{
		autoLoginTimer.setSingleShot(true);
		autoLoginTimer.setInterval(100);
		connect(&autoLoginTimer, &QTimer::timeout, this, &LiveLoginDialog::login);
		autoLoginTimer.start();
	}
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

bool LiveLoginDialog::checkAccountPasswordAcceptableInput() const
{
	ui.loginPushButton->setEnabled(ui.accountLineEdit->hasAcceptableInput()
		&& ui.passwordLineEdit->hasAcceptableInput());
	return ui.loginPushButton->isEnabled();
}
