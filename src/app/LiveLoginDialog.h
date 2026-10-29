#pragma once

#include <QDesktopServices>
#include <QToolButton>
#include <QMessageBox>
#include <QUrl>
#include <QCompleter>
#include <QRegExpValidator>
#include <QStyle>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "LiveLoginSettingDialog.h"
#include "DragableDialog.h"
#include "ShadowWidget.h"
#include "utils.h"
#include "ui_LiveLoginDialog.h"

class LiveLoginDialog : public DragableDialog
{
	Q_OBJECT

public:
	LiveLoginDialog(QWidget* parent = Q_NULLPTR);
protected:
	void login();
	void handleLoginReply(QNetworkReply* reply, nlohmann::json loginPackage,
		std::string account, bool tokenLogin);
	void handleProfileReply(QNetworkReply* reply, const std::string& account);
	void finishLoginAttempt();
	void applyConfig();
	void setAvatar();
	void setAccountCompleter();
	void setAccountPasswordAcceptableInputCheck();
	bool checkAccountPasswordAcceptableInput() const;

protected:
	LiveLoginSettingDialog settingDialog{ this };
	ShadowWidget shadowWidget{ this };
	QTimer autoLoginTimer{ this };
	QNetworkAccessManager networkManager;
	bool loginInProgress{ false };

private:
	Ui::LiveLoginDialogClass ui;
};
