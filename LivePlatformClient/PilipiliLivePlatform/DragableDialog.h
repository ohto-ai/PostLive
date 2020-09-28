#pragma once

#include "BaseDialog.h"

class DragableDialog : public BaseDialog
{
protected:
	DragableDialog(QWidget* parent, Qt::WindowFlags f = 0);
public:
	virtual ~DragableDialog();

protected:
	virtual void mousePressEvent(QMouseEvent* event) override;
	virtual void mouseReleaseEvent(QMouseEvent* event) override;
	virtual void mouseMoveEvent(QMouseEvent* event) override;
protected:
	bool mouseDown{ false };
	QPoint mouseLastPos;
};
