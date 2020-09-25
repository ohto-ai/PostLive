#pragma once

#include <QtWidgets/QDialog>
#include <QDesktopServices>
#include <QToolButton>
#include <QMessageBox>
#include <QUrl>
#include <QCompleter>
#include <QRegExpValidator>
#include <QStyle>
#include <QMouseEvent>
#include <QCloseEvent>
#include "utils.h"
#include "ui_LiveLoginDialog.h"

class LiveLoginDialog : public QDialog
{
    Q_OBJECT

public:
    LiveLoginDialog(QWidget *parent = Q_NULLPTR);

    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void closeEvent(QCloseEvent* event) override;
protected:
    bool mouseIsPressed{ false };
    QPoint pLast;
    QTimer errorLabelTimer;
protected:
    void login();
    void saveDataBeforeLog();
    void touchCheatPassword(QString);
    void applyConfig();
    void setAvatar();
    void setAccountCompleter();
    void setAccountPasswordAcceptableInputCheck();
    bool checkAccountPasswordAcceptableInput();
private:
    Ui::LiveLoginDialogClass ui;
};
