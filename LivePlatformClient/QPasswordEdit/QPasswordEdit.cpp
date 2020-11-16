#include "QPasswordEdit.h"
#include <QRegExpValidator>
#include <QCryptographicHash>
#include <QMouseEvent>

QPasswordEdit::QPasswordEdit(QWidget* parent)
	: QLineEdit(parent)
{
	QLineEdit::setEchoMode(EchoMode::Password);
	connect(this, &QPasswordEdit::focusIn, [&]
		{
			if (type == CheatPasswordFilled)
				QLineEdit::clear();
		});
	connect(this, &QPasswordEdit::focusOut, [&]
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

void QPasswordEdit::mousePressEvent(QMouseEvent* e)
{
	if (e->button() & Qt::LeftButton) {
		emit leftClicked();
		emit clicked();
	}
	else if (e->button() & Qt::RightButton)
	{
		emit rightClicked();
		emit clicked();
	}
	else if (e->button() & Qt::MiddleButton)
	{
		emit middleClicked();
		emit clicked();
	}
	QLineEdit::mousePressEvent(e);
}

void QPasswordEdit::focusInEvent(QFocusEvent* e)
{
	emit focusIn();
	QLineEdit::focusInEvent(e);
}

void QPasswordEdit::focusOutEvent(QFocusEvent* e)
{
	emit focusOut();
	QLineEdit::focusOutEvent(e);
}


QPasswordEdit::PasswordType QPasswordEdit::passwordType() const
{
	return type;
}

void QPasswordEdit::fillPassword(QString pwd)
{
	type = PasswordFilled;
	setText(pwd);
	pwdMD5 = QCryptographicHash::hash(pwd.toUtf8()
		, QCryptographicHash::Md5).toHex().toUpper();
}

void QPasswordEdit::fillCheatPassword(QString pwd, QString md5)
{
	type = CheatPasswordFilled;
	setText(pwd);
	cheatPwd = pwd;
	pwdMD5 = md5;
}

void QPasswordEdit::setRegExpValidator(const QRegExp& regexp)
{
	setValidator(new QRegExpValidator(regexp, this));
}

QString QPasswordEdit::md5() const
{
	return pwdMD5;
}

QString QPasswordEdit::password() const
{
	return text();
}

void QPasswordEdit::clear()
{
	QLineEdit::clear();
	type = PasswordFilled;
}
