#pragma once

#include <QWidget>

class ShadowWidget : public QWidget
{
	Q_OBJECT

public:
	void show();
	ShadowWidget(QWidget *parent);
	~ShadowWidget();

	void showShadowWindow(QWidget*);
protected:
	virtual void mousePressEvent(QMouseEvent* e)override;
};
