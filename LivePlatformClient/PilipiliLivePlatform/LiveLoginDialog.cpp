#pragma execution_character_set("utf-8")

#include "LiveLoginDialog.h"


LiveLoginDialog::LiveLoginDialog(QWidget* parent)
    : QDialog(parent)
{
    ui.setupUi(this);
    applyConfig();
}

void LiveLoginDialog::mousePressEvent(QMouseEvent* event)
{
    if (Qt::LeftButton == event->button() && !isMaximized())
    {
        ui.loginPushButton->setFocus();
        pLast = event->globalPos();
        event->ignore();
    }
    mouseIsPressed = true;
}

void LiveLoginDialog::mouseReleaseEvent(QMouseEvent* event)
{
    QApplication::restoreOverrideCursor();
    event->ignore();

    mouseIsPressed = false;

}

void LiveLoginDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (mouseIsPressed && !isMaximized() && (event->buttons() & Qt::LeftButton))
    {
        move(event->globalPos() + pos() - pLast);
        pLast = event->globalPos();
    }
}

void LiveLoginDialog::closeEvent(QCloseEvent* event)
{
    thatboy::utils::saveUsers();
    thatboy::storage::config["login"]["geometry"] = geometry();
    thatboy::utils::saveConfig();
}

void LiveLoginDialog::login()
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
        QMessageBox::information(this, "Unable to connect to server.", "Error");
    else
    {
        close();
        QMessageBox::information(this, "login", "login success.", QMessageBox::Ok);
    }
}

void LiveLoginDialog::saveDataBeforeLog()
{
    auto& loginConfig = thatboy::storage::config["login"];
    loginConfig["current_user"]["name"] = ui.accountLineEdit->text();
    loginConfig["remember_password"] = ui.rememberPasswordCheckBox->isChecked();
    loginConfig["auto_login"] = ui.autoLoginCheckBox->isChecked();
}

void LiveLoginDialog::touchCheatPassword()
{
    disconnect(ui.accountLineEdit, &QLineEdit::textChanged, this, &LiveLoginDialog::touchCheatPassword);
    disconnect(ui.passwordLineEdit, &QLineEdit::textChanged, this, &LiveLoginDialog::touchCheatPassword);
    if (thatboy::storage::pswdFromConfig)
    {
        ui.passwordLineEdit->clear();
        thatboy::storage::pswdFromConfig = false;
    }
}

void LiveLoginDialog::fillCheatPassword()
{
    auto& loginConfig = thatboy::storage::config["login"];
    if (thatboy::storage::pswdFromConfig)
        ui.passwordLineEdit->setText(QString("%1")
            .arg("a", loginConfig["current_user"]["pwmask"].get<int>(), QLatin1Char('a')));
    connect(ui.accountLineEdit, &QLineEdit::textChanged, this, &LiveLoginDialog::touchCheatPassword);
    connect(ui.passwordLineEdit, &QLineEdit::textChanged, this, &LiveLoginDialog::touchCheatPassword);
}

void LiveLoginDialog::applyConfig()
{
    auto& loginConfig = thatboy::storage::config["login"];
    setGeometry(loginConfig["geometry"]);
    ui.accountLineEdit->setText(loginConfig["current_user"]["name"]);
    ui.rememberPasswordCheckBox->setChecked(loginConfig["remember_password"]);
    ui.autoLoginCheckBox->setChecked(loginConfig["auto_login"]);

    thatboy::storage::pswdFromConfig = loginConfig["remember_password"];
    fillCheatPassword();

    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    connect(ui.registerPushButton, &QPushButton::clicked
        , std::bind(&QDesktopServices::openUrl, QUrl(thatboy::storage::RegisterUrl)));
    connect(ui.forgetPasswordPushButton, &QPushButton::clicked
        , std::bind(&QDesktopServices::openUrl, QUrl(thatboy::storage::ResetPasswordUrl)));


    connect(ui.loginPushButton, &QPushButton::clicked, this, &LiveLoginDialog::login);
    connect(ui.accountLineEdit, &QLineEdit::textChanged, this, &LiveLoginDialog::setAvatar);
    connect(ui.minimizeToolButton, &QToolButton::clicked, this, &QDialog::showMinimized);
    connect(ui.closeToolButton, &QToolButton::clicked, this, &QDialog::close);

    setAvatar();
    setAccountCompleter();
    setAccountPasswordAcceptableInputCheck();
}

void LiveLoginDialog::setAvatar()
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
    {
        if (thatboy::storage::userAvatarCache.find(userName) == thatboy::storage::userAvatarCache.end())
        {

        }
        avatar = thatboy::utils::roundedPixmap(QPixmap{
            thatboy::storage::users[userName]["avatar_file"].get<QString>() });
        if (avatar.isNull())
        {
            thatboy::storage::userAvatarCache
        }
    }
    if (!avatar.isNull())
        ui.avatarLabel->setPixmap(avatar);
    else
        ui.avatarLabel->setPixmap(defaultAvatarGroup[userName.length() / 5 % 3]);

}

void LiveLoginDialog::setAccountCompleter()
{
    auto& completer = *new QCompleter(thatboy::storage::users["user_names"], this);
    completer.setFilterMode(Qt::MatchFlag::MatchContains);
    ui.accountLineEdit->setCompleter(&completer);
}

void LiveLoginDialog::setAccountPasswordAcceptableInputCheck()
{
    connect(ui.accountLineEdit, &QLineEdit::textChanged
        , this, &LiveLoginDialog::checkAccountPasswordAcceptableInput);
    connect(ui.passwordLineEdit, &QLineEdit::textChanged
        , this, &LiveLoginDialog::checkAccountPasswordAcceptableInput);
    ui.accountLineEdit->setValidator(new QRegExpValidator(
        QRegExp{ thatboy::storage::RegexAccount }, this));
    ui.passwordLineEdit->setValidator(new QRegExpValidator(
        QRegExp{ thatboy::storage::RegexPassword }, this));
    checkAccountPasswordAcceptableInput();
}

bool LiveLoginDialog::checkAccountPasswordAcceptableInput()
{
    ui.loginPushButton->setEnabled(ui.accountLineEdit->hasAcceptableInput()
        && ui.passwordLineEdit->hasAcceptableInput());
    return ui.loginPushButton->isEnabled();
}
