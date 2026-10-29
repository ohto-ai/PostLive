#pragma once
// QComboBox that reports when its popup is about to open, so the caller can
// refresh the item list right before display (used for the window picker).
#include <QComboBox>

class WindowComboBox : public QComboBox
{
	Q_OBJECT

public:
	using QComboBox::QComboBox;

signals:
	void aboutToShowPopup();

protected:
	void showPopup() override
	{
		emit aboutToShowPopup();
		QComboBox::showPopup();
	}
};
