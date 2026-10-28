#pragma once

#include <QLineEdit>

class TouchedLineEdit : public QLineEdit
{
	Q_OBJECT

public:
	TouchedLineEdit(QWidget* parent);
	~TouchedLineEdit();
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
};
