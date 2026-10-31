#include "FileDropZone.h"

#include <QDragEnterEvent>
#include <QMimeData>
#include <QUrl>

FileDropZone::FileDropZone(QWidget* parent)
	: QWidget(parent)
{
	setAcceptDrops(true);
}

void FileDropZone::dropEvent(QDropEvent* event)
{
	const QMimeData* mimeData = event->mimeData();
	if (mimeData->hasUrls())
	{
		QStringList paths;
		for (const QUrl& url : mimeData->urls())
			paths.push_back(url.toLocalFile());
		emit onFileDropped(paths);
	}
}

void FileDropZone::dragEnterEvent(QDragEnterEvent* event)
{
	if (event->mimeData()->hasUrls())
		event->acceptProposedAction();
}
