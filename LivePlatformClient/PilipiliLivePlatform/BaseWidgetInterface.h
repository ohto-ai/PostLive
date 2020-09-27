#pragma once

class BaseWidgetInterface
{
public:
	virtual void showed() = 0;
	virtual void hided() = 0;
	virtual void closed() = 0;
	virtual void moved() = 0;
	virtual void resized() = 0;
	virtual void clicked() = 0;
	virtual void leftClicked() = 0;
	virtual void rightClicked() = 0;
	virtual void middleClicked() = 0;
};