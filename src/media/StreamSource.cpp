#include "StreamSource.h"

#include <QSize>

QString StreamSource::displayTitle() const
{
	switch (kind)
	{
	case Kind::Camera:
		return QCoreApplication::translate("StreamSource", "摄像头: %1").arg(title.isEmpty() ? name : title);
	case Kind::Desktop:
		return QCoreApplication::translate("StreamSource", "屏幕: %1").arg(title.isEmpty() ? name : title);
	case Kind::Window:
		return QCoreApplication::translate("StreamSource", "窗口: %1").arg(name);
	case Kind::LocalFile:
		return QCoreApplication::translate("StreamSource", "文件: %1").arg(name);
	}
	return name;
}

void to_json(nlohmann::json& j, const StreamSource& source)
{
	j = {
		{ "kind", static_cast<int>(source.kind) },
		{ "name", source.name.toStdString() },
		{ "title", source.title.toStdString() },
		{ "x", source.rect.x() },
		{ "y", source.rect.y() },
		{ "width", source.rect.width() },
		{ "height", source.rect.height() },
	};
}

void from_json(const nlohmann::json& j, StreamSource& source)
{
	const int kind = j.value("kind", 0);
	source.kind = (kind >= 0 && kind <= 3)
		? static_cast<StreamSource::Kind>(kind)
		: StreamSource::Kind::Camera;
	source.name = QString::fromStdString(j.value("name", std::string()));
	source.title = QString::fromStdString(j.value("title", std::string()));
	const int x = j.value("x", 0);
	const int y = j.value("y", 0);
	const int width = j.value("width", 0);
	const int height = j.value("height", 0);
	source.rect = QRect(x, y, width, height);
}
