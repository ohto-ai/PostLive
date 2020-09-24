#include "utils.h"

QPixmap thatboy::utils::roundedPixmap(const QPixmap& src)
{
    if (src.isNull()) {
        return QPixmap();
    }
    int radius = std::max(src.width(), src.height());
    QSize size(2 * radius, 2 * radius);
    QBitmap mask(size);
    QPainter painter(&mask);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.fillRect(0, 0, size.width(), size.height(), Qt::white);
    painter.setBrush(QColor(0, 0, 0));
    painter.drawRoundedRect(0, 0, size.width(), size.height(), radius * 2 - 1, radius * 2 - 1);
    QPixmap image = src.scaled(size);
    image.setMask(mask);
    return image;
}

void thatboy::utils::from_json(const nlohmann::json& j, QString& string)
{
    string = QString::fromStdString(j.get<std::string>());
}

void thatboy::utils::from_json(const nlohmann::json& j, QStringList& stringList)
{
    stringList.clear();
    for (auto& str : j)
        stringList.push_back(str);
}

void thatboy::utils::from_json(const nlohmann::json& j, QRect& rect)
{
    rect.setRect(j["x"], j["y"], j["width"], j["height"]);
}

void thatboy::from_json(const nlohmann::json& j, TimeStampToken& token)
{
    token.token = j["token"].get<QString>();
    token.timeStamp = j["stamp"];
}

void thatboy::utils::to_json(nlohmann::json& j, const QString& string)
{
    j = string.toStdString();
}

void thatboy::utils::to_json(nlohmann::json& j, const QStringList& stringList)
{
    j.clear();
    for (auto& str : stringList)
        j.push_back(str.toStdString());
}

void thatboy::utils::to_json(nlohmann::json& j, const QRect& rect)
{
    j["x"] = rect.x();
    j["y"] = rect.y();
    j["width"] = rect.width();
    j["height"] = rect.height();
}

void thatboy::to_json(nlohmann::json& j, const TimeStampToken& token)
{
    j["token"] = token.token;
    j["stamp"] = token.timeStamp;
}

void thatboy::utils::loadUsers()
{
    std::ifstream ifs(storage::config["user_list"].get<std::string>());
    storage::users = nlohmann::json::parse(std::string{ (std::istreambuf_iterator<char>(ifs)),
        (std::istreambuf_iterator<char>()) });    
}

void thatboy::utils::saveUsers()
{
    std::ofstream ofs(storage::config["user_list"].get<std::string>());
    ofs << storage::users.dump(4);
}

thatboy::TimeStampToken thatboy::utils::generateTrustedToken(QString password)
{
    return TimeStampToken(password);
}

void thatboy::utils::loadConfig()
{
    std::ifstream ifs(storage::ConfigFilePath);
    storage::config = nlohmann::json::parse(std::string{ (std::istreambuf_iterator<char>(ifs)),
        (std::istreambuf_iterator<char>()) });    
}

void thatboy::utils::saveConfig()
{
    std::ofstream ofs(storage::ConfigFilePath);
    ofs << storage::config.dump(4);
}

nlohmann::json thatboy::storage::users;
nlohmann::json thatboy::storage::config;
nlohmann::json thatboy::storage::currentUser;
bool thatboy::storage::pswdFromConfig{ false };
httplib::Client thatboy::storage::accountVerifyClient{ "localhost",8080 };