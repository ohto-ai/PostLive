#pragma once
#include <QPoint>
#include <QSize>

class BaseWidgetInterface
{
public:
	virtual void showed() = 0;
	virtual void hided() = 0;
	virtual void closed() = 0;
	virtual void moved(const QPoint&, const QPoint&) = 0;
	virtual void resized(const QSize&, const QSize&) = 0;
	virtual void clicked() = 0;
	virtual void leftClicked() = 0;
	virtual void rightClicked() = 0;
	virtual void middleClicked() = 0;
	virtual void focusIn() = 0;
	virtual void focusOut() = 0;
};