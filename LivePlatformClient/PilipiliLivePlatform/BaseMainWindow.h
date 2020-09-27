#pragma once

#include "BaseWidgetInterface.h"
#include <QMainWindow>

class BaseMainWindow 
	: public QMainWindow
	, public BaseWidgetInterface
{
	Q_OBJECT

public:
	BaseMainWindow(QWidget* parent, Qt::WindowFlags f = 0);
	~BaseMainWindow();

protected:
	virtual void showEvent(QShowEvent*) override;
	virtual void hideEvent(QHideEvent*) override;
	virtual void closeEvent(QCloseEvent*) override;
	virtual void moveEvent(QMoveEvent*) override;
	virtual void resizeEvent(QResizeEvent*) override;
	virtual void mousePressEvent(QMouseEvent*) override;
signals:
	void showed();
	void hided();
	void closed();
	void moved();
	void resized();
	void clicked();
	void leftClicked();
	void rightClicked();
	void middleClicked();
};
