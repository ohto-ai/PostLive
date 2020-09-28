#include "PasswordEdit.h"
#include <QRegExpValidator>
#include <QCryptographicHash>
#include <QMouseEvent>

PasswordEdit::PasswordType PasswordEdit::passwordType() const
{
	return type;
}

void PasswordEdit::fillPassword(QString pwd)
{
	type = PasswordFilled;
	setText(pwd);
	pwdMD5 = QCryptographicHash::hash(pwd.toUtf8()
		, QCryptographicHash::Md5).toHex().toUpper();
}

void PasswordEdit::fillCheatPassword(QString pwd, QString md5)
{
	type = CheatPasswordFilled;
	setText(pwd);
	cheatPwd = pwd;
	pwdMD5 = md5;
}

void PasswordEdit::setRegExpValidator(const QRegExp& regexp)
{
	setValidator(new QRegExpValidator(regexp, this));
}

QString PasswordEdit::md5() const
{
	return pwdMD5;
}

QString PasswordEdit::password() const
{
	return text();
}

void PasswordEdit::clear()
{
	QLineEdit::clear();
	type = PasswordFilled;
}

PasswordEdit::PasswordEdit(QWidget* parent)
	: TouchedLineEdit(parent)
{
	connect(this, &TouchedLineEdit::clicked, [&]
		{
			if (type == CheatPasswordFilled)
				QLineEdit::clear();
		});
	connect(this, &TouchedLineEdit::focusOut, [&]
		{
			if (type == CheatPasswordFilled)
				setText(cheatPwd);
		});
	connect(this, &QLineEdit::textEdited, [&]
		{
			type = PasswordEdited;
		});
	connect(this, &QLineEdit::editingFinished, [&]
		{
			if (type != CheatPasswordFilled)
				pwdMD5 = QCryptographicHash::hash(password().toUtf8()
					, QCryptographicHash::Md5).toHex().toUpper();
		});
}

PasswordEdit::~PasswordEdit()
{
}