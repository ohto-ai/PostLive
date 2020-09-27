#pragma once

#include "TouchedLineEdit.h"

class PasswordEdit : public TouchedLineEdit
{
	Q_OBJECT

public:
	enum PasswordType
	{
		PasswordEdited,
		PasswordFilled,
		CheatPasswordFilled
	};
public:
	PasswordType passwordType()const;
	void fillPassword(QString);
	void fillCheatPassword(QString, QString);
	void setRegExpValidator(const QRegExp&);
	QString md5()const;
	QString password()const;
	void clear();

	PasswordEdit(QWidget *parent);
	~PasswordEdit();

protected:
	PasswordType type{ PasswordEdited };
	QString cheatPwd;
	QString pwdMD5;
};
