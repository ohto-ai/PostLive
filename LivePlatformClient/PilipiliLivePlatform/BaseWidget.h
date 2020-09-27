#pragma once

#include "BaseWidgetInterface.h"
#include <QWidget>

class BaseWidget 
	: public QWidget
	, public BaseWidgetInterface
{
	Q_OBJECT

public:
	BaseWidget(QWidget* parent, Qt::WindowFlags f = 0);
	~BaseWidget();

protected:
	virtual void showEvent(QShowEvent*) override;
	virtual void hideEvent(QHideEvent*) override;
	virtual void closeEvent(QCloseEvent*) override;
	virtual void moveEvent(QMoveEvent*) override;
	virtual void resizeEvent(QResizeEvent*) override;
	virtual void mousePressEvent(QMouseEvent*) override;
	virtual void focusInEvent(QFocusEvent*) override;
	virtual void focusOutEvent(QFocusEvent*) override;
signals:
	void showed();
	void hided();
	void closed();
	void moved(const QPoint&, const QPoint&);
	void resized(const QSize&, const QSize&);
	void clicked();
	void leftClicked();
	void rightClicked();
	void middleClicked();
	void focusIn();
	void focusOut();
};
