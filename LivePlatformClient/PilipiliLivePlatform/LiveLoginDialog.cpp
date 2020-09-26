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
    thatboy::utils::saveUserData();
    thatboy::storage::config["login"]["geometry"] = geometry();
    thatboy::utils::saveConfig();
}

void LiveLoginDialog::login()
{
    saveDataBeforeLog();
    using thatboy::storage::currentUser;
    using thatboy::storage::usersStorage;

    std::string account = ui.accountLineEdit->text().toStdString();

    nlohmann::json loginPackage;
    loginPackage["account"] = account;
    loginPackage["type"] = "login";
    if (thatboy::storage::usingToken)
    {
        loginPackage["token"] = usersStorage["users"][account]["token"];
        loginPackage["login"] = "token";
    }
    else
    {
        loginPackage["password"] = thatboy::utils::generateMD5(ui.passwordLineEdit->text());
        loginPackage["login"] = "password";
    }
    // 发送
    auto res = thatboy::storage::accountVerifyClient.Post("/api", loginPackage.dump(), "application/json");
    
    if (!res)
        QMessageBox::warning(this, "Warning", "无法连接到服务器.");
    else
    {
        auto body{ nlohmann::json::parse(res->body) };
        if (!body["success"])
            QMessageBox::warning(this, "Warning", "登录失败." + body["info"].get<QString>());
        else
        {
            currentUser["account"] = account;
            currentUser["token"] = body["token"];
            usersStorage["users"][account]["token"] = body["token"];
            usersStorage["users"][account]["pwmask"] = ui.passwordLineEdit->text().length();
            usersStorage["users"][account]["remember_password"] = ui.rememberPasswordCheckBox->isChecked();

            loginPackage["type"] = "profile";
            loginPackage["profile"].push_back("avatar");
            loginPackage["profile"].push_back("gender");
            loginPackage["token"] = currentUser["token"];
            res = thatboy::storage::accountVerifyClient.Post("/api", loginPackage.dump(), "application/json");
            body = nlohmann::json::parse(res->body);
                        
            usersStorage["users"][account]["avatar"] = body["profile"]["avatar"];
            usersStorage["users"][account]["gender"] = body["profile"]["gender"];

            accept();
        }
    }
}

void LiveLoginDialog::saveDataBeforeLog()
{
    auto& loginConfig = thatboy::storage::config["login"];
    loginConfig["current_user"] = ui.accountLineEdit->text();
    loginConfig["auto_login"] = ui.autoLoginCheckBox->isChecked();
}

void LiveLoginDialog::touchCheatPassword(QString str)
{
    disconnect(ui.passwordLineEdit, &QLineEdit::textEdited, this, &LiveLoginDialog::touchCheatPassword);
    if (thatboy::storage::usingToken)
    {
        thatboy::storage::usingToken = false;
        if (str.length() < thatboy::storage::usersStorage["users"][ui.accountLineEdit->text().toStdString()]["pwmask"].get<int>())// 删除
            ui.passwordLineEdit->clear();
        else
        {
            str.remove('a');
            if (str.isEmpty())
                str = 'a';
            ui.passwordLineEdit->setText(str);
        }
    }
}

void LiveLoginDialog::applyConfig()
{
    auto& loginConfig = thatboy::storage::config["login"];
    setGeometry(loginConfig["geometry"]);

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
    connect(&thatboy::storage::generalSignal, &GeneralSignal::avatarDownloaded, [=](QString user, bool succcess)
        {
            if (succcess && user == ui.accountLineEdit->text())
                setAvatar();
        });

    setAvatar();
    setAccountCompleter();
    setAccountPasswordAcceptableInputCheck();

    ui.autoLoginCheckBox->setChecked(loginConfig["auto_login"]);

    connect(ui.accountLineEdit, &QLineEdit::textChanged, [&] {
        ui.passwordLineEdit->clear();
        using thatboy::storage::usersStorage;
        auto account = ui.accountLineEdit->text().toStdString();
        if (usersStorage["users"].contains(account))
        {
            ui.rememberPasswordCheckBox->setChecked(usersStorage["users"][account]["remember_password"]);
            thatboy::storage::usingToken = usersStorage["users"][account]["remember_password"];

            if (usersStorage["users"][account]["remember_password"])
                ui.passwordLineEdit->setText(QString("%1")
                    .arg("a", usersStorage["users"][account]["pwmask"].get<int>(), QLatin1Char('a')));
            connect(ui.passwordLineEdit, &QLineEdit::textEdited, this, &LiveLoginDialog::touchCheatPassword);
            connect(ui.passwordLineEdit, &QLineEdit::selectionChanged, [&] 
                {
                    if(thatboy::storage::usingToken)
                        ui.passwordLineEdit->deselect();
                });
        }
        });
    ui.accountLineEdit->setText(loginConfig["current_user"]);
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
    if (thatboy::storage::usersStorage["users"].contains(userName) && !thatboy::storage::userAvatarCache[userName].isNull())
        avatar = thatboy::utils::roundedPixmap(thatboy::storage::userAvatarCache[userName]);
    if (!avatar.isNull())
        ui.avatarLabel->setPixmap(avatar);
    else
        ui.avatarLabel->setPixmap(defaultAvatarGroup[userName.length() / 5 % 3]);

}

void LiveLoginDialog::setAccountCompleter()
{
    auto& completer = *new QCompleter(thatboy::storage::usersStorage["user_names"], this);
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
