#pragma once
// Composited scene preview: runs one FFmpegVideo decode thread per source,
// paints the first source as the base layer and every following source as a
// picture-in-picture layer anchored to the bottom-right corner. Dropping a
// media file adds it as a local-file source.
#include "FFmpegInputDevice.h"
#include "../widgets/FileDropZone.h"

#include <QHash>
#include <QImage>
#include <QMovie>
#include <QMutex>

#include "StreamSource.h"

class FFmpegVideo;

class ScenePreview : public FileDropZone
{
	Q_OBJECT

public:
	ScenePreview(QWidget* parent = nullptr);
	~ScenePreview() override;

	// Reconciles the running decode threads with the given scene: sources
	// that are kept keep playing, added ones start decoding, removed ones
	// are stopped.
	void setSources(const QList<StreamSource>& sources);
	void stopAll();

	void paintEvent(QPaintEvent* event) override;

signals:
	void sourceError(const QString& message);

private:
	struct RenderSlot
	{
		quint64 id = 0;
		StreamSource source;
		FFmpegInputDevice device;             // stable address handed to the decoder
		FFmpegVideo* video = nullptr;
		QImage frame;
		bool frameAvailable = false;
	};

	QList<RenderSlot*> m_slots;               // owned; index == z-order
	QMutex m_frameMutex;
	QMovie m_movie{ ":/main/res/splash.gif" };
};
