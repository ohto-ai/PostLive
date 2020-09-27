#pragma once

#include <QDesktopServices>
#include <QToolButton>
#include <QMessageBox>
#include <QUrl>
#include <QCompleter>
#include <QRegExpValidator>
#include <QStyle>
#include <QMouseEvent>
#include <QCloseEvent>
#include "DragableDialog.h"
#include "utils.h"
#include "ui_LiveLoginDialog.h"

class LiveLoginDialog : public DragableDialog
{
    Q_OBJECT

public:
    LiveLoginDialog(QWidget *parent = Q_NULLPTR);
protected:
    virtual void closeEvent(QCloseEvent* event) override;
protected:
    void login();
    void applyConfig();
    void setAvatar();
    void setAccountCompleter();
    void setAccountPasswordAcceptableInputCheck();
    bool checkAccountPasswordAcceptableInput();
private:
    Ui::LiveLoginDialogClass ui;
};
