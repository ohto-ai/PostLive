#pragma once

#include <QtWidgets/QLineEdit>

class QPasswordEdit : public QLineEdit
{
public:
	enum PasswordType
	{
		PasswordEdited,
		PasswordFilled,
		CheatPasswordFilled
	};
	
	Q_OBJECT
	Q_PROPERTY(QString password WRITE fillPassword)

public:
	
	PasswordType passwordType()const;
	void fillPassword(QString);
	void fillCheatPassword(QString, QString);
	void setRegExpValidator(const QRegExp&);
	QString md5()const;
	QString password()const;
	void clear();

	QPasswordEdit(QWidget* parent);
	~QPasswordEdit() = default;

protected:
	virtual void mousePressEvent(QMouseEvent* e) override;
	virtual void focusInEvent(QFocusEvent* e) override;
	virtual void focusOutEvent(QFocusEvent* e) override;
signals:
	void clicked();
	void leftClicked();
	void rightClicked();
	void middleClicked();

	void focusIn();
	void focusOut();
protected:
	PasswordType type{ PasswordEdited };
	QString cheatPwd;
	QString pwdMD5;
};
