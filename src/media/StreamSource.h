#pragma once
// One entry of a broadcast scene. The scene is an ordered list: the first
// source is the base layer, every following source is stacked on top of it
// (picture-in-picture during streaming, z-ordered in preview).
#include <QCoreApplication>
#include <QList>
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
	QString name;   // camera device name / "desktop" / window title / file path
	QString title;  // display title

	bool operator==(const StreamSource& other) const
	{
		return kind == other.kind && name == other.name;
	}

	QString displayTitle() const;
};

// JSON persistence for config["scene"]["sources"].
void to_json(nlohmann::json& j, const StreamSource& source);
void from_json(const nlohmann::json& j, StreamSource& source);
