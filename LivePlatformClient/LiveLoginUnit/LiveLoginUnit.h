#pragma once

#include <QtWidgets/QMainWindow>
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
#include "ui_LiveLoginUnit.h"

class LiveLoginUnit : public QMainWindow
{
    Q_OBJECT

public:
    LiveLoginUnit(QWidget *parent = Q_NULLPTR);

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
    void setErrorLabel(QString);
    void saveDataBeforeLog();
    void touchCheatPassword();
    void fillCheatPassword();
    void applyFramelessWindow();
    void applyConfig();
    void setAvatar();
    void setAccountCompleter();
    void setAccountPasswordAcceptableInputCheck();
    bool checkAccountPasswordAcceptableInput();
private:
    Ui::LiveLoginUnitClass ui;
};
