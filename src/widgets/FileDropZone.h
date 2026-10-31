#pragma once

#include <QWidget>

// Accepts file drops and reports the local paths. Base class of ScenePreview:
// dropping a media file onto the preview adds it as a source.
class FileDropZone : public QWidget
{
	Q_OBJECT

public:
	FileDropZone(QWidget* parent = Q_NULLPTR);

signals:
	void onFileDropped(const QStringList& paths);

protected:
	void dropEvent(QDropEvent* event) override;
	void dragEnterEvent(QDragEnterEvent* event) override;
};
