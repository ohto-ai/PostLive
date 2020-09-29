#include "WidgetConfigInfo.h"

using thatboy::from_json;
using thatboy::to_json;

thatboy::WidgetConfigInfo::WidgetConfigInfo(const QWidget& widget)
	: geometry(widget.geometry())
	, opacity(widget.windowOpacity())
	, translucent(widget.testAttribute(Qt::WA_TranslucentBackground))
{}

void thatboy::WidgetConfigInfo::assign(const QWidget& widget)
{
	geometry = widget.geometry();
	opacity = widget.windowOpacity();
	translucent = widget.testAttribute(Qt::WA_TranslucentBackground);
}

void thatboy::WidgetConfigInfo::config(QWidget& widget) const
{
	widget.setGeometry(geometry);
	widget.setWindowOpacity(opacity);
	widget.setAttribute(Qt::WA_TranslucentBackground, translucent);
}

void thatboy::from_json(const nlohmann::json& j, WidgetConfigInfo& w)
{
	w.geometry = j["geometry"];
	w.opacity = j["opacity"];
	w.translucent = j["translucent"];
}

void thatboy::to_json(nlohmann::json& j, const WidgetConfigInfo& w)
{
	j["geometry"] = w.geometry;
	j["opacity"] = w.opacity;
	j["translucent"] = w.translucent;
}

void thatboy::from_json(const nlohmann::json& j, QRect& rect)
{
	rect.setRect(j["x"], j["y"], j["width"], j["height"]);
}


void thatboy::to_json(nlohmann::json& j, const QRect& rect)
{
	j["x"] = rect.x();
	j["y"] = rect.y();
	j["width"] = rect.width();
	j["height"] = rect.height();
}
