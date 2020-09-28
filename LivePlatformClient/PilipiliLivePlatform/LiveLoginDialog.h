#pragma once

#include <QDesktopServices>
#include <QToolButton>
#include <QMessageBox>
#include <QUrl>
#include <QCompleter>
#include <QRegExpValidator>
#include <QStyle>
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
	void applyConfig();
	void setAvatar();
	void setAccountCompleter();
	void setAccountPasswordAcceptableInputCheck();
	bool checkAccountPasswordAcceptableInput();

protected:
	LiveLoginSettingDialog settingDialog{ this };
	ShadowWidget shadowWidget{ this };

private:
	Ui::LiveLoginDialogClass ui;
};
