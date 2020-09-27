#pragma once

#include <QDialog>
#include "ui_TestWindow.h"

class TestWindow : public QDialog
{
	Q_OBJECT

public:
	TestWindow(QWidget *parent = Q_NULLPTR);
	~TestWindow();

private:
	Ui::TestWindow ui;
};
