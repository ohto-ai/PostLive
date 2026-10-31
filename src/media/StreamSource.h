#pragma once
// One entry of a broadcast scene. The scene is an ordered list: the first
// source is the base layer, every following source is stacked on top of it.
// Each source carries its rect in canvas coordinates (the canvas is the
// stream output size, 16:9 by default); an invalid rect means "auto layout".
#include <QCoreApplication>
#include <QList>
#include <QRect>
#include <QString>
#include <nlohmann/json.hpp>

struct StreamSource
{
	enum class Kind
	{
		Camera,
		Desktop,
		Window,
		LocalFile,
	};

	Kind kind{ Kind::Camera };
	QString name;   // camera device name / monitor device name / window title / file path
	QString title;  // display title
	QRect rect;     // position+size in canvas pixels; invalid = auto layout

	bool operator==(const StreamSource& other) const
	{
		return kind == other.kind && name == other.name;
	}

	QString displayTitle() const;
};

// JSON persistence for config["scene"]["sources"].
void to_json(nlohmann::json& j, const StreamSource& source);
void from_json(const nlohmann::json& j, StreamSource& source);
