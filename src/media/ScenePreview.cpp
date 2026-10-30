#include "ScenePreview.h"

#include "FFmpegInputDevice.h"
#include "FFmpegVideo.h"
#include "../core/Logging.h"

#include <QPainter>

namespace
{
	quint64 nextSlotId()
	{
		static quint64 id = 0;
		return ++id;
	}
}

ScenePreview::ScenePreview(QWidget* parent)
	: FileDropZone(parent)
{
	QPalette pal = palette();
	pal.setColor(QPalette::Window, Qt::black);
	setAutoFillBackground(true);
	setPalette(pal);

	if (m_movie.isValid())
	{
		connect(&m_movie, &QMovie::frameChanged, this, [this]
			{
				update();
			});
		m_movie.start();
	}

	// Dropping a media file adds it as a local-file source.
	connect(this, &FileDropZone::onFileDropped, this, [this](const QStringList& paths)
		{
			if (paths.isEmpty())
				return;
			QList<StreamSource> sources;
			for (const auto& slot : m_slots)
				sources.append(slot->source);
			StreamSource dropped;
			dropped.kind = StreamSource::Kind::LocalFile;
			dropped.name = paths.front();
			sources.append(dropped);
			setSources(sources);
		});
}

ScenePreview::~ScenePreview()
{
	stopAll();
	qDeleteAll(m_slots);
	m_slots.clear();
}

void ScenePreview::setSources(const QList<StreamSource>& sources)
{
	// Reconcile slots with the requested scene. Slots are matched by
	// (kind, name); matching slots keep their decode thread running and are
	// reordered, added ones start decoding, removed ones are stopped.
	QList<RenderSlot*> nextSlots;
	for (const StreamSource& source : sources)
	{
		RenderSlot* kept = nullptr;
		for (RenderSlot* slot : m_slots)
		{
			if (slot->source == source)
			{
				kept = slot;
				break;
			}
		}
		if (kept != nullptr)
		{
			m_slots.removeOne(kept);
			nextSlots.append(kept);
			continue;
		}

		auto* slot = new RenderSlot;
		slot->id = nextSlotId();
		slot->source = source;
		switch (source.kind)
		{
		case StreamSource::Kind::Camera:
			for (const FFmpegInputDevice* device : FFmpegInputDevice::filterVideoDevices())
			{
				if (QString::fromStdString(device->device_name) == source.name)
				{
					slot->device = *device;
					break;
				}
			}
			break;
		case StreamSource::Kind::Desktop:
			slot->device = FFmpegInputDevice::make_gdi_device();
			break;
		case StreamSource::Kind::Window:
			slot->device = FFmpegInputDevice::make_gdi_device(source.name.toStdString());
			break;
		case StreamSource::Kind::LocalFile:
			slot->device = FFmpegInputDevice::make_local_file_device(source.name.toStdString());
			break;
		}
		nextSlots.append(slot);
	}

	// Stop the decode threads of dropped sources before destroying them.
	for (RenderSlot* slot : m_slots)
	{
		if (slot->video != nullptr)
		{
			slot->video->stop();
			delete slot->video;
		}
		delete slot;
	}
	m_slots = nextSlots;

	for (RenderSlot* slot : m_slots)
	{
		if (slot->video == nullptr)
		{
			slot->video = new FFmpegVideo(this);
			if (!slot->video->setInputDevice(&slot->device))
			{
				qCWarning(lcStream) << "preview source rejected:" << slot->source.displayTitle();
				delete slot->video;
				slot->video = nullptr;
				continue;
			}
			const quint64 id = slot->id;
			connect(slot->video, &FFmpegVideo::frameReady, this,
				[this, id](const QImage& frame)
				{
					RenderSlot* target = nullptr;
					for (RenderSlot* slot : m_slots)
					{
						if (slot->id == id)
						{
							target = slot;
							break;
						}
					}
					if (target == nullptr)
						return; // source was removed while the event was queued
					QMutexLocker lock(&m_frameMutex);
					target->frame = frame;
					target->frameAvailable = true;
					m_movie.stop();
					update();
				});
			connect(slot->video, &FFmpegVideo::postError, this,
				[this, id](int ec, const QString& message)
				{
					qCWarning(lcStream) << "preview source error:" << id << ec << message;
					for (RenderSlot* slot : m_slots)
					{
						if (slot->id == id)
						{
							QMutexLocker lock(&m_frameMutex);
							slot->frameAvailable = false;
							slot->frame = QImage();
							break;
						}
					}
					update();
				});
			slot->video->start();
		}
	}
	update();
}

void ScenePreview::stopAll()
{
	for (RenderSlot* slot : m_slots)
	{
		if (slot->video != nullptr)
		{
			slot->video->stop();
			delete slot->video;
			slot->video = nullptr;
		}
		slot->frame = QImage();
		slot->frameAvailable = false;
	}
}

void ScenePreview::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	painter.fillRect(rect(), Qt::black);

	QList<QImage> frames;
	{
		QMutexLocker lock(&m_frameMutex);
		for (const RenderSlot* slot : m_slots)
			frames.append(slot->frame);
	}

	if (frames.isEmpty() || frames.front().isNull())
	{
		if (!m_movie.isValid() || m_movie.state() != QMovie::Running)
		{
			painter.setPen(Qt::white);
			painter.drawText(rect(), Qt::AlignCenter,
				QCoreApplication::translate("ScenePreview", "从左侧添加采集源开始"));
			return;
		}
		const QImage splash = m_movie.currentImage()
			.scaled(width(), height(), Qt::KeepAspectRatio);
		painter.drawImage((width() - splash.width()) / 2, (height() - splash.height()) / 2, splash);
		return;
	}

	// Base layer: keep aspect ratio, centered.
	const QImage& base = frames.front();
	const QImage scaledBase = base.scaled(width(), height(), Qt::KeepAspectRatio);
	painter.drawImage((width() - scaledBase.width()) / 2, (height() - scaledBase.height()) / 2, scaledBase);

	// Overlay layers: bottom-right, stacked with a small offset so several
	// picture-in-picture sources stay distinguishable.
	const int pipWidth = width() / 4;
	for (int i = 1; i < frames.size(); ++i)
	{
		if (frames[i].isNull())
			continue;
		const QImage pip = frames[i].scaledToWidth(pipWidth, Qt::SmoothTransformation);
		const int offset = (i - 1) * 16;
		painter.drawImage(width() - pip.width() - 12 - offset,
			height() - pip.height() - 12 - offset, pip);
	}
}
