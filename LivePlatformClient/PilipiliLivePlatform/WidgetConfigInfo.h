#pragma once

#include <json.hpp>
#include <QWidget>

namespace thatboy
{
	class WidgetConfigInfo
	{
	public:
		WidgetConfigInfo() = default;
		WidgetConfigInfo(const WidgetConfigInfo&) = default;
		WidgetConfigInfo(WidgetConfigInfo&&) = default;
		WidgetConfigInfo(const QWidget&);
		WidgetConfigInfo& operator =(const WidgetConfigInfo&) = default;
		WidgetConfigInfo& operator =(WidgetConfigInfo&&) = default;

		void assign(const QWidget&);
		void config(QWidget&) const;

		friend void from_json(const nlohmann::json&, WidgetConfigInfo&);
		friend void to_json(nlohmann::json&, const WidgetConfigInfo&);
	protected:
		QRect geometry;
		qreal opacity;
		bool translucent;
	};
	void from_json(const nlohmann::json& j, QRect& rect);
	void to_json(nlohmann::json& j, const QRect& rect);
}