#pragma execution_character_set("utf-8")

#include "LiveLoginUnit.h"


LiveLoginUnit::LiveLoginUnit(QWidget* parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    applyConfig();
    applyFramelessWindow();
}

void LiveLoginUnit::mousePressEvent(QMouseEvent* event)
{
    if (Qt::LeftButton == event->button() && !isMaximized())
    {
        ui.loginPushButton->setFocus();
        pLast = event->globalPos();
        event->ignore();
    }
    mouseIsPressed = true;
}

void LiveLoginUnit::mouseReleaseEvent(QMouseEvent* event)
{
    QApplication::restoreOverrideCursor();
    event->ignore();

    mouseIsPressed = false;

}

void LiveLoginUnit::mouseMoveEvent(QMouseEvent* event)
{
    if (mouseIsPressed && !isMaximized() && (event->buttons() & Qt::LeftButton))
    {
        move(event->globalPos() + pos() - pLast);
        pLast = event->globalPos();
    }
}

void LiveLoginUnit::closeEvent(QCloseEvent* event)
{
    thatboy::utils::saveUsers();
    thatboy::storage::config["login"]["geometry"] = geometry();
    thatboy::utils::saveConfig();
}

void LiveLoginUnit::login()
{
    saveDataBeforeLog();
    auto& loginConfig = thatboy::storage::config["login"];
    // generate pswd
    if (!thatboy::storage::pswdFromConfig)
    {
        loginConfig["current_user"]["trusted_token"] = thatboy::utils::generateTrustedToken(ui.passwordLineEdit->text());
        loginConfig["current_user"]["pwmask"] = ui.passwordLineEdit->text().length();
    }

    // try connect to server and verify identity.
    auto res = thatboy::storage::accountVerifyClient.Post("/verify", loginConfig["current_user"]["trusted_token"].dump(), "application/json");
    if (!res)
        setErrorLabel("Unable to connect to server");
    else
    {
        close();
        QMessageBox::information(this, "login", "login success.", QMessageBox::Ok);
    }
}

void LiveLoginUnit::setErrorLabel(QString err)
{
    ui.errorLabel->setText(err);
    ui.errorLabel->show();
    errorLabelTimer.start(thatboy::storage::LoginErrorLabelTime);
}

void LiveLoginUnit::saveDataBeforeLog()
{
    auto& loginConfig = thatboy::storage::config["login"];
    loginConfig["current_user"]["name"] = ui.accountLineEdit->text();
    loginConfig["remember_password"] = ui.rememberPasswordCheckBox->isChecked();
    loginConfig["auto_login"] = ui.autoLoginCheckBox->isChecked();
}

void LiveLoginUnit::touchCheatPassword()
{
    disconnect(ui.accountLineEdit, &QLineEdit::textChanged, this, &LiveLoginUnit::touchCheatPassword);
    disconnect(ui.passwordLineEdit, &QLineEdit::textChanged, this, &LiveLoginUnit::touchCheatPassword);
    if (thatboy::storage::pswdFromConfig)
    {
        ui.passwordLineEdit->clear();
        thatboy::storage::pswdFromConfig = false;
    }
}

void LiveLoginUnit::fillCheatPassword()
{
    auto& loginConfig = thatboy::storage::config["login"];
    if (thatboy::storage::pswdFromConfig)
        ui.passwordLineEdit->setText(QString("%1")
            .arg("a", loginConfig["current_user"]["pwmask"].get<int>(), QLatin1Char('a')));
    connect(ui.accountLineEdit, &QLineEdit::textChanged, this, &LiveLoginUnit::touchCheatPassword);
    connect(ui.passwordLineEdit, &QLineEdit::textChanged, this, &LiveLoginUnit::touchCheatPassword);
}

void LiveLoginUnit::applyFramelessWindow()
{
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    // set tool buttons
    ui.minimizeToolButton->setIcon(style()->standardPixmap(QStyle::SP_TitleBarMinButton));
    ui.closeToolButton->setIcon(style()->standardPixmap(QStyle::SP_TitleBarCloseButton));
    connect(ui.minimizeToolButton, &QToolButton::clicked, this, &QMainWindow::showMinimized);
    connect(ui.closeToolButton, &QToolButton::clicked, this, &QMainWindow::close);

    // hide errorlabel
    ui.errorLabel->hide();
    connect(&errorLabelTimer, &QTimer::timeout, ui.errorLabel, &QLabel::hide);
    connect(ui.errorLabel, &TouchedLabel::clicked, ui.errorLabel, &QLabel::hide);
    setAvatar();
    setAccountCompleter();
    setAccountPasswordAcceptableInputCheck();
}

void LiveLoginUnit::applyConfig()
{
    auto& loginConfig = thatboy::storage::config["login"];
    setGeometry(loginConfig["geometry"]);
    ui.accountLineEdit->setText(loginConfig["current_user"]["name"]);
    ui.rememberPasswordCheckBox->setChecked(loginConfig["remember_password"]);
    ui.autoLoginCheckBox->setChecked(loginConfig["auto_login"]);

    thatboy::storage::pswdFromConfig = loginConfig["remember_password"];
    fillCheatPassword();

    connect(ui.registerPushButton, &QPushButton::clicked
        , std::bind(&QDesktopServices::openUrl, QUrl(thatboy::storage::RegisterUrl)));
    connect(ui.forgetPasswordPushButton, &QPushButton::clicked
        , std::bind(&QDesktopServices::openUrl, QUrl(thatboy::storage::ResetPasswordUrl)));


    connect(ui.loginPushButton, &QPushButton::clicked, this, &LiveLoginUnit::login);
    connect(ui.accountLineEdit, &QLineEdit::textChanged, this, &LiveLoginUnit::setAvatar);
}

void LiveLoginUnit::setAvatar()
{
    static const QPixmap defaultAvatarGroup[3]
    {
        thatboy::storage::DefaultAvatarUrlGroup[0]
        , thatboy::storage::DefaultAvatarUrlGroup[1]
        , thatboy::storage::DefaultAvatarUrlGroup[2]
    };
    auto userName = ui.accountLineEdit->text().toStdString();
    QPixmap avatar;
    if (thatboy::storage::users.contains(userName))
        avatar = thatboy::utils::roundedPixmap(QPixmap{
        thatboy::storage::users[userName]["avatar_file"].get<QString>() });
    else
        avatar = defaultAvatarGroup[userName.length() / 5 % 3];
    if (!avatar.isNull())
        ui.avatarLabel->setPixmap(avatar);
}

void LiveLoginUnit::setAccountCompleter()
{
    auto& completer = *new QCompleter(thatboy::storage::users["user_names"], this);
    completer.setFilterMode(Qt::MatchFlag::MatchContains);
    ui.accountLineEdit->setCompleter(&completer);
}

void LiveLoginUnit::setAccountPasswordAcceptableInputCheck()
{
    connect(ui.accountLineEdit, &QLineEdit::textChanged
        , this, &LiveLoginUnit::checkAccountPasswordAcceptableInput);
    connect(ui.passwordLineEdit, &QLineEdit::textChanged
        , this, &LiveLoginUnit::checkAccountPasswordAcceptableInput);
    ui.accountLineEdit->setValidator(new QRegExpValidator(
        QRegExp{ thatboy::storage::RegexAccount }, this));
    ui.passwordLineEdit->setValidator(new QRegExpValidator(
        QRegExp{ thatboy::storage::RegexPassword }, this));
    checkAccountPasswordAcceptableInput();
}

bool LiveLoginUnit::checkAccountPasswordAcceptableInput()
{
    ui.loginPushButton->setEnabled(ui.accountLineEdit->hasAcceptableInput()
        && ui.passwordLineEdit->hasAcceptableInput());
    return ui.loginPushButton->isEnabled();
}
