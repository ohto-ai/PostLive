#include "ScenePreview.h"

#include "FFmpegInputDevice.h"
#include "FFmpegVideo.h"
#include "capture/DesktopDuplicationCapture.h"
#include "capture/GraphicsCapture.h"
#include "capture/WindowPrintCapture.h"
#include "../core/Logging.h"
#include "../core/utils.h"

#include <QContextMenuEvent>
#include <QFileInfo>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>

namespace
{
	constexpr int MinSourceSize = 40;
	constexpr qreal HandleSize = 10.0;      // corner resize handle, canvas px
	constexpr qreal SnapDistance = 10.0;    // canvas px, center/edge snapping
	constexpr qreal GuidePenWidth = 1.0;

	quint64 nextSlotId()
	{
		static quint64 id = 0;
		return ++id;
	}

	bool hasVideoProviderFor(const StreamSource& source)
	{
		return source.kind == StreamSource::Kind::Camera
			|| source.kind == StreamSource::Kind::LocalFile;
	}
}

ScenePreview::ScenePreview(QWidget* parent)
	: FileDropZone(parent)
{
	QPalette pal = palette();
	pal.setColor(QPalette::Window, Qt::black);
	setAutoFillBackground(true);
	setPalette(pal);
	setMouseTracking(true);
	setFocusPolicy(Qt::ClickFocus);

	if (m_movie.isValid())
	{
		connect(&m_movie, &QMovie::frameChanged, this, [this]
			{
				update();
			});
		m_movie.start();
	}

	// Compose the canvas at ~30 fps; this is both the preview content and
	// (while streaming) the frame feed for the in-process encoder.
	connect(&m_composeTimer, &QTimer::timeout, this, &ScenePreview::composeCanvas);
	m_composeTimer.start(33);

	// Dropping a media file adds it as a local-file source.
	connect(this, &FileDropZone::onFileDropped, this, [this](const QStringList& paths)
		{
			if (paths.isEmpty())
				return;
			QList<StreamSource> sources;
			for (const RenderSlot* slot : m_slots)
				sources.append(slot->source);
			StreamSource dropped;
			dropped.kind = StreamSource::Kind::LocalFile;
			dropped.name = paths.front();
			sources.append(dropped);
			setSources(sources);
		});
}

void ScenePreview::setCanvasSize(const QSize& size)
{
	if (size.width() <= 0 || size.height() <= 0 || size == m_canvasSize)
		return;
	m_canvasSize = size;
	ensureDefaultRects();
	composeCanvas();
}

ScenePreview::~ScenePreview()
{
	stopAll();
	qDeleteAll(m_slots);
	m_slots.clear();
}

ScenePreview::RenderSlot* ScenePreview::findSlot(quint64 id)
{
	for (RenderSlot* slot : m_slots)
	{
		if (slot->id == id)
			return slot;
	}
	return nullptr;
}

void ScenePreview::setSources(const QList<StreamSource>& sources)
{
	// Reconcile slots with the requested scene. Slots are matched by
	// (kind, name); matching slots keep their provider running and are
	// reordered, added ones start, removed ones stop.
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

	for (RenderSlot* slot : m_slots)
	{
		if (slot->video != nullptr)
		{
			slot->video->stop();
			delete slot->video;
		}
		if (auto* wgc = qobject_cast<GraphicsCapture*>(slot->capture))
			wgc->stop();
		if (auto* desktop = qobject_cast<DesktopDuplicationCapture*>(slot->capture))
			desktop->stop();
		if (auto* window = qobject_cast<WindowPrintCapture*>(slot->capture))
			window->stop();
		delete slot->capture;
		delete slot;
	}
	m_slots = nextSlots;

	ensureDefaultRects();
	for (RenderSlot* slot : m_slots)
	{
		if (slot->video == nullptr && slot->capture == nullptr)
			startSlot(slot);
	}

	if (m_selectedIndex >= m_slots.size())
		m_selectedIndex = -1;
	update();
}

void ScenePreview::startSlot(RenderSlot* slot)
{
	const quint64 id = slot->id;

	if (hasVideoProviderFor(slot->source))
	{
		slot->video = new FFmpegVideo(this);
		if (!slot->video->setInputDevice(&slot->device))
		{
			qCWarning(lcStream) << "preview source rejected:" << slot->source.displayTitle();
			delete slot->video;
			slot->video = nullptr;
			return;
		}
	}
	else
	{
		// WGC (OBS's modern method) captures DWM-composited content —
		// hardware-accelerated windows and fullscreen games — where the GDI
		// and duplication paths return black frames. Fall back to the older
		// engines on systems without the 1903+ APIs.
		const bool useWgc = GraphicsCapture::isSupported();
		switch (slot->source.kind)
		{
		case StreamSource::Kind::Desktop:
		{
			qCInfo(lcStream) << "desktop capture provider:" << (useWgc ? "WGC" : "DXGI");
			slot->capture = useWgc
				? static_cast<QThread*>(new GraphicsCapture(slot->source.name,
					GraphicsCapture::Target::Monitor, 30, this))
				: new DesktopDuplicationCapture(slot->source.name, 30, this);
			break;
		}
		case StreamSource::Kind::Window:
		{
			slot->capture = useWgc
				? static_cast<QThread*>(new GraphicsCapture(slot->source.name,
					GraphicsCapture::Target::Window, 30, this))
				: new WindowPrintCapture(slot->source.name, 30, this);
			break;
		}
		default:
			return;
		}
	}

	auto onFrame = [this, id](QImage frame)
	{
		RenderSlot* target = findSlot(id);
		if (target == nullptr)
			return; // source was removed while the event was queued

		// First frame: snap the auto-layout rect to the real aspect ratio.
		if (!frame.isNull() && !target->aspectAdjusted)
		{
			target->aspectAdjusted = true;
			const qreal aspect = qreal(frame.width()) / qreal(frame.height());
			QRect rect = target->source.rect;
			if (!rect.isValid())
				rect = QRect(0, 0, m_canvasSize.width(), m_canvasSize.height());
			rect.setHeight(int(qreal(rect.width()) / aspect));
			rect.setHeight(qBound(MinSourceSize, rect.height(), m_canvasSize.height()));
			target->source.rect = rect;
		}

		QMutexLocker lock(&m_frameMutex);
		target->frame = frame;
		m_movie.stop();
	};

	if (slot->video != nullptr)
	{
		connect(slot->video, &FFmpegVideo::frameReady, this, onFrame);
		connect(slot->video, &FFmpegVideo::postError, this,
			[this, id](int ec, const QString& message)
			{
				qCWarning(lcStream) << "preview source error:" << id << ec << message;
				RenderSlot* target = findSlot(id);
				if (target != nullptr)
				{
					QMutexLocker lock(&m_frameMutex);
					target->frame = QImage();
				}
				update();
			});
		slot->video->start();
	}
	else if (slot->capture != nullptr)
	{
		connect(slot->capture, &QThread::finished, this, [this, id]
			{
				// Capture threads deleteLater on finish; drop the frame.
				RenderSlot* target = findSlot(id);
				if (target != nullptr)
				{
					QMutexLocker lock(&m_frameMutex);
					target->frame = QImage();
				}
			});
		// QThread subclasses cannot use new-style signal syntax through the
		// base pointer; cast to the concrete types.
		if (auto* wgc = qobject_cast<GraphicsCapture*>(slot->capture))
			connect(wgc, &GraphicsCapture::frameReady, this, onFrame);
		else if (auto* desktop = qobject_cast<DesktopDuplicationCapture*>(slot->capture))
			connect(desktop, &DesktopDuplicationCapture::frameReady, this, onFrame);
		else if (auto* window = qobject_cast<WindowPrintCapture*>(slot->capture))
			connect(window, &WindowPrintCapture::frameReady, this, onFrame);
		slot->capture->start(QThread::HighPriority);
	}
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
		if (auto* wgc = qobject_cast<GraphicsCapture*>(slot->capture))
			wgc->stop();
		if (auto* desktop = qobject_cast<DesktopDuplicationCapture*>(slot->capture))
			desktop->stop();
		if (auto* window = qobject_cast<WindowPrintCapture*>(slot->capture))
			window->stop();
		delete slot->capture;
		slot->capture = nullptr;
		QMutexLocker lock(&m_frameMutex);
		slot->frame = QImage();
	}
}

void ScenePreview::ensureDefaultRects()
{
	for (int i = 0; i < m_slots.size(); ++i)
	{
		QRect& rect = m_slots[i]->source.rect;
		if (rect.isValid() && !rect.isEmpty())
			continue;
		if (i == 0)
			rect = QRect(QPoint(0, 0), m_canvasSize);
		else
		{
			const int width = m_canvasSize.width() / 4;
			const int height = width * 9 / 16;
			rect = QRect(m_canvasSize.width() - width - 24,
				m_canvasSize.height() - height - 24, width, height);
		}
	}
}

void ScenePreview::composeCanvas()
{
	if (m_slots.isEmpty() && !m_movie.isValid())
		return;

	m_canvas = QImage(m_canvasSize, QImage::Format_ARGB32);
	m_canvas.fill(Qt::black);
	QPainter painter(&m_canvas);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);

	bool anyFrame = false;
	for (const RenderSlot* slot : m_slots)
	{
		QImage frame;
		{
			QMutexLocker lock(&m_frameMutex);
			frame = slot->frame;
		}
		if (frame.isNull())
			continue;
		anyFrame = true;
		painter.drawImage(slot->source.rect, frame);
	}

	if (thatboy::storage::config["stream"].value("watermark", true))
	{
		const QString watermarkFile = QStringLiteral("assets/pilipili-logo.png");
		if (QFileInfo::exists(watermarkFile))
		{
			QPixmap watermark(watermarkFile);
			if (!watermark.isNull())
				painter.drawPixmap(8, 8, 120,
					int(120 * qreal(watermark.height()) / watermark.width()), watermark);
		}
	}
	painter.end();

	if (!anyFrame)
	{
		// No source has delivered anything yet: keep the canvas but let the
		// paint layer show the loading animation instead.
		m_canvas = QImage();
	}
	emit canvasReady(m_canvas);
	update();
}

void ScenePreview::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	painter.fillRect(rect(), Qt::black);

	const QRectF canvas = canvasRectInWidget();
	if (m_canvas.isNull())
	{
		if (!m_movie.isValid() || m_movie.state() != QMovie::Running)
		{
			painter.setPen(Qt::white);
			painter.drawText(rect(), Qt::AlignCenter,
				QCoreApplication::translate("ScenePreview", "从左侧“＋”添加采集源开始"));
			return;
		}
		const QImage splash = m_movie.currentImage().scaled(
			int(canvas.width()), int(canvas.height()), Qt::KeepAspectRatio);
		painter.drawImage(int(canvas.left() + (canvas.width() - splash.width()) / 2),
			int(canvas.top() + (canvas.height() - splash.height()) / 2), splash);
		return;
	}

	painter.drawImage(canvas, m_canvas);

	// Selection overlay: drawn in widget space so it never reaches the
	// streamed canvas.
	if (m_selectedIndex >= 0 && m_selectedIndex < m_slots.size())
	{
		const QRectF source = sourceRectInWidget(m_slots[m_selectedIndex]->source.rect);
		painter.setPen(QPen(QColor(0, 160, 230), 2));
		painter.setBrush(Qt::NoBrush);
		painter.drawRect(source);

		// Corner resize handles.
		painter.setPen(Qt::NoPen);
		painter.setBrush(QColor(0, 160, 230));
		for (const QPointF& corner : { source.topLeft(), source.topRight(),
			source.bottomLeft(), source.bottomRight() })
			painter.drawRect(QRectF(corner.x() - HandleSize / 2, corner.y() - HandleSize / 2,
				HandleSize, HandleSize));

		// Snapping guides (move mode only).
		if (m_dragMode == DragMode::Move)
		{
			painter.setPen(QPen(QColor(0, 160, 230), GuidePenWidth, Qt::DashLine));
			if (m_snapH)
				painter.drawLine(QPointF(canvas.left(), canvas.center().y()),
					QPointF(canvas.right(), canvas.center().y()));
			if (m_snapV)
				painter.drawLine(QPointF(canvas.center().x(), canvas.top()),
					QPointF(canvas.center().x(), canvas.bottom()));
		}
	}
}

QRectF ScenePreview::canvasRectInWidget() const
{
	const qreal scale = qMin(qreal(width()) / m_canvasSize.width(),
		qreal(height()) / m_canvasSize.height());
	const qreal w = m_canvasSize.width() * scale;
	const qreal h = m_canvasSize.height() * scale;
	return QRectF((width() - w) / 2, (height() - h) / 2, w, h);
}

QPointF ScenePreview::toCanvasCoords(const QPointF& widgetPos) const
{
	const QRectF canvas = canvasRectInWidget();
	return QPointF((widgetPos.x() - canvas.left()) / canvas.width() * m_canvasSize.width(),
		(widgetPos.y() - canvas.top()) / canvas.height() * m_canvasSize.height());
}

QRectF ScenePreview::sourceRectInWidget(const QRect& rect) const
{
	const QRectF canvas = canvasRectInWidget();
	const qreal scale = canvas.width() / m_canvasSize.width();
	return QRectF(canvas.left() + rect.x() * scale, canvas.top() + rect.y() * scale,
		rect.width() * scale, rect.height() * scale);
}

int ScenePreview::hitTest(const QPointF& canvasPos) const
{
	// Topmost (last) first; sources without a frame are not selectable.
	for (int i = m_slots.size() - 1; i >= 0; --i)
	{
		if (m_slots[i]->frame.isNull())
			continue;
		if (m_slots[i]->source.rect.contains(canvasPos.toPoint()))
			return i;
	}
	return -1;
}

int ScenePreview::resizeCornerHitTest(const QRect& rect, const QPointF& canvasPos) const
{
	// Corner handles live in widget space so they keep a constant screen size.
	const QRectF widgetRect = sourceRectInWidget(rect);
	const QPointF corners[4] = {
		widgetRect.topLeft(), widgetRect.topRight(),
		widgetRect.bottomLeft(), widgetRect.bottomRight()
	};
	for (int i = 0; i < 4; ++i)
	{
		if (QRectF(corners[i].x() - HandleSize / 2, corners[i].y() - HandleSize / 2,
			HandleSize, HandleSize).contains(canvasPos))
			return i; // matches ResizeCorner TopLeft/TopRight/BottomLeft/BottomRight
	}
	return -1;
}

void ScenePreview::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		const QPointF canvasPos = toCanvasCoords(event->pos());
		const int hit = hitTest(canvasPos);
		m_selectedIndex = hit;
		emit sourceSelected(m_selectedIndex);

		if (m_selectedIndex < 0)
		{
			m_dragMode = DragMode::None;
			update();
			return FileDropZone::mousePressEvent(event);
		}

		const QRect rect = m_slots[m_selectedIndex]->source.rect;
		const int corner = resizeCornerHitTest(rect, event->pos());
		if (corner >= 0)
		{
			m_dragMode = DragMode::Resize;
			m_resizeCorner = ResizeCorner(corner);
			m_dragStartRect = rect;
			setCursor((corner == 0 || corner == 3) ? Qt::SizeFDiagCursor : Qt::SizeBDiagCursor);
		}
		else
		{
			m_dragMode = DragMode::Move;
			m_dragGrabOffset = canvasPos - QPointF(rect.topLeft());
			setCursor(Qt::SizeAllCursor);
		}
		setFocus(Qt::MouseFocusReason);
		update();
		return;
	}
	FileDropZone::mousePressEvent(event);
}

void ScenePreview::mouseMoveEvent(QMouseEvent* event)
{
	const QPointF canvasPos = toCanvasCoords(event->pos());

	if (m_dragMode == DragMode::None)
	{
		// Hover cursor feedback.
		const int hit = hitTest(canvasPos);
		if (hit < 0)
			setCursor(Qt::ArrowCursor);
		else
		{
			const int corner = resizeCornerHitTest(m_slots[hit]->source.rect, event->pos());
			if (corner == 0 || corner == 3)
				setCursor(Qt::SizeFDiagCursor);
			else if (corner == 1 || corner == 2)
				setCursor(Qt::SizeBDiagCursor);
			else
				setCursor(Qt::SizeAllCursor);
		}
		return FileDropZone::mouseMoveEvent(event);
	}

	RenderSlot* slot = m_selectedIndex >= 0 && m_selectedIndex < m_slots.size()
		? m_slots[m_selectedIndex] : nullptr;
	if (slot == nullptr)
		return;

	QRect rect = slot->source.rect;
	m_snapH = m_snapV = false;
	if (m_dragMode == DragMode::Move)
	{
		const QPointF topLeft = canvasPos - m_dragGrabOffset;
		rect.moveTo(int(topLeft.x()), int(topLeft.y()));

		// Center snapping with guides, OBS-style.
		const int canvasCenterX = m_canvasSize.width() / 2;
		const int canvasCenterY = m_canvasSize.height() / 2;
		if (qAbs(rect.center().x() - canvasCenterX) <= SnapDistance)
		{
			rect.moveLeft(canvasCenterX - rect.width() / 2);
			m_snapH = true;
		}
		if (qAbs(rect.center().y() - canvasCenterY) <= SnapDistance)
		{
			rect.moveTop(canvasCenterY - rect.height() / 2);
			m_snapV = true;
		}
	}
	else
	{
		// Aspect-locked corner resize: the opposite corner stays anchored and
		// overshooting the canvas edge clamps cleanly instead of shrinking.
		const qreal aspect = qMax(0.01, qreal(m_dragStartRect.width())
			/ qMax(1, m_dragStartRect.height()));
		QPointF anchor;
		bool growRight = false;
		bool growDown = false;
		switch (m_resizeCorner)
		{
		case ResizeCorner::TopLeft:
			anchor = QPointF(rect.right(), rect.bottom());
			growRight = canvasPos.x() < anchor.x();
			growDown = canvasPos.y() < anchor.y();
			break;
		case ResizeCorner::TopRight:
			anchor = QPointF(rect.left(), rect.bottom());
			growRight = canvasPos.x() >= anchor.x();
			growDown = canvasPos.y() < anchor.y();
			break;
		case ResizeCorner::BottomLeft:
			anchor = QPointF(rect.right(), rect.top());
			growRight = canvasPos.x() < anchor.x();
			growDown = canvasPos.y() >= anchor.y();
			break;
		default:
			anchor = QPointF(rect.left(), rect.top());
			growRight = canvasPos.x() >= anchor.x();
			growDown = canvasPos.y() >= anchor.y();
			break;
		}

		const qreal dx = canvasPos.x() - anchor.x();
		const qreal dy = canvasPos.y() - anchor.y();
		int newWidth;
		int newHeight;
		if (qAbs(dx) / aspect >= qAbs(dy))
		{
			newWidth = qBound(MinSourceSize, int(qAbs(dx)), m_canvasSize.width());
			newHeight = int(newWidth / aspect);
		}
		else
		{
			newHeight = qBound(MinSourceSize, int(qAbs(dy)), m_canvasSize.height());
			newWidth = int(newHeight * aspect);
		}
		newWidth = qMin(newWidth, growRight
			? m_canvasSize.width() - int(anchor.x())
			: int(anchor.x()));
		newHeight = qMin(newHeight, growDown
			? m_canvasSize.height() - int(anchor.y())
			: int(anchor.y()));
		newWidth = qMax(MinSourceSize, newWidth);
		newHeight = qMax(MinSourceSize, newHeight);

		const int x = growRight ? int(anchor.x()) : int(anchor.x()) - newWidth;
		const int y = growDown ? int(anchor.y()) : int(anchor.y()) - newHeight;
		rect = QRect(qBound(0, x, m_canvasSize.width()),
			qBound(0, y, m_canvasSize.height()), newWidth, newHeight);
	}

	if (m_dragMode == DragMode::Move)
	{
		// Keep the source fully inside the canvas without ever touching its
		// size here: the old width/height-forcing clamp shrank sources that
		// were dragged across the right/bottom edge.
		rect.moveLeft(qBound(0, rect.x(), qMax(0, m_canvasSize.width() - rect.width())));
		rect.moveTop(qBound(0, rect.y(), qMax(0, m_canvasSize.height() - rect.height())));
	}
	slot->source.rect = rect;
	// Live preview of the transform; persistence happens on mouse release.
	composeCanvas();
}

void ScenePreview::mouseReleaseEvent(QMouseEvent* event)
{
	if (m_dragMode != DragMode::None)
	{
		m_dragMode = DragMode::None;
		m_snapH = m_snapV = false;
		setCursor(Qt::ArrowCursor);
		QList<StreamSource> sources;
		for (const RenderSlot* slot : m_slots)
			sources.append(slot->source);
		emit sceneChanged(sources);
	}
	FileDropZone::mouseReleaseEvent(event);
}

void ScenePreview::mouseDoubleClickEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		const int hit = hitTest(toCanvasCoords(event->pos()));
		if (hit >= 0)
		{
			centerSourceHorizontally(hit);
			return;
		}
	}
	FileDropZone::mouseDoubleClickEvent(event);
}

void ScenePreview::contextMenuEvent(QContextMenuEvent* event)
{
	const int hit = hitTest(toCanvasCoords(event->pos()));
	if (hit < 0)
		return FileDropZone::contextMenuEvent(event);

	m_selectedIndex = hit;
	emit sourceSelected(m_selectedIndex);
	update();

	QMenu menu(this);
	menu.addAction(tr("适配画布"), this, [this, hit]
		{
			fitSource(hit);
		});
	menu.addAction(tr("水平居中"), this, [this, hit]
		{
			centerSourceHorizontally(hit);
		});
	menu.addSeparator();
	menu.addAction(tr("上移一层"), this, [this, hit]
		{
			if (hit + 1 < m_slots.size())
			{
				m_slots.swapItemsAt(hit, hit + 1);
				QList<StreamSource> sources;
				for (const RenderSlot* slot : m_slots)
					sources.append(slot->source);
				composeCanvas();
				emit sceneChanged(sources);
			}
		});
	menu.addAction(tr("下移一层"), this, [this, hit]
		{
			if (hit - 1 >= 0)
			{
				m_slots.swapItemsAt(hit, hit - 1);
				QList<StreamSource> sources;
				for (const RenderSlot* slot : m_slots)
					sources.append(slot->source);
				composeCanvas();
				emit sceneChanged(sources);
			}
		});
	menu.addSeparator();
	menu.addAction(tr("移除源"), this, [this, hit]
		{
			emit removeSourceRequested(hit);
		});
	menu.exec(event->globalPos());
}

void ScenePreview::keyPressEvent(QKeyEvent* event)
{
	if (m_selectedIndex < 0 || m_selectedIndex >= m_slots.size())
		return FileDropZone::keyPressEvent(event);

	RenderSlot* slot = m_slots[m_selectedIndex];
	const int step = (event->modifiers() & Qt::ShiftModifier) ? 10 : 1;
	QRect rect = slot->source.rect;
	switch (event->key())
	{
	case Qt::Key_Left:
		rect.translate(-step, 0);
		break;
	case Qt::Key_Right:
		rect.translate(step, 0);
		break;
	case Qt::Key_Up:
		rect.translate(0, -step);
		break;
	case Qt::Key_Down:
		rect.translate(0, step);
		break;
	case Qt::Key_Delete:
	case Qt::Key_Backspace:
		emit removeSourceRequested(m_selectedIndex);
		return;
	default:
		return FileDropZone::keyPressEvent(event);
	}
	rect.moveLeft(qBound(0, rect.x(), qMax(0, m_canvasSize.width() - rect.width())));
	rect.moveTop(qBound(0, rect.y(), qMax(0, m_canvasSize.height() - rect.height())));
	slot->source.rect = rect;
	composeCanvas();
}

void ScenePreview::fitSource(int index)
{
	if (index < 0 || index >= m_slots.size())
		return;
	m_slots[index]->source.rect = QRect(QPoint(0, 0), m_canvasSize);
	composeCanvas();
	QList<StreamSource> sources;
	for (const RenderSlot* slot : m_slots)
		sources.append(slot->source);
	emit sceneChanged(sources);
}

void ScenePreview::centerSourceHorizontally(int index)
{
	if (index < 0 || index >= m_slots.size())
		return;
	QRect& rect = m_slots[index]->source.rect;
	rect.moveLeft((m_canvasSize.width() - rect.width()) / 2);
	composeCanvas();
	QList<StreamSource> sources;
	for (const RenderSlot* slot : m_slots)
		sources.append(slot->source);
	emit sceneChanged(sources);
}

void ScenePreview::resizeEvent(QResizeEvent* event)
{
	FileDropZone::resizeEvent(event);
	update();
}

void ScenePreview::setSelectedIndex(int index)
{
	m_selectedIndex = (index >= 0 && index < m_slots.size()) ? index : -1;
	update();
}
