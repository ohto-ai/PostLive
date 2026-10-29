#pragma once

#include <QLineEdit>
#include "utils.h"
#include "DragableDialog.h"
#include "ui_LiveLoginSettingDialog.h"

class LiveLoginSettingDialog : public DragableDialog
{
	Q_OBJECT

public:
	LiveLoginSettingDialog(QWidget* parent = Q_NULLPTR);
	~LiveLoginSettingDialog();

private:
	void save();
	// Marks a field with a red border and shows the reason inline instead of
	// opening a modal popup; pass a null field to clear.
	void showFieldError(QLineEdit* field, const QString& message);

private:
	Ui::LiveLoginSettingDialog ui;
	QLineEdit* errorField{ nullptr };
};
