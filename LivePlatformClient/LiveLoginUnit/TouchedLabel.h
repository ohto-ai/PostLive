#pragma once

#include <QLabel>
#include <QMouseEvent>

class TouchedLabel : public QLabel
{
	Q_OBJECT

public:
	TouchedLabel(QWidget *parent);
	~TouchedLabel();

protected:
	virtual void mousePressEvent(QMouseEvent* e) override;
signals:
	void clicked();
	void leftClicked();
	void rightClicked();
	void middleClicked();
};
