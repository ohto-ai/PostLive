#pragma once
// OBS-style composited preview: every source is fed by its own provider
// (FFmpegVideo for cameras/files, DXGI duplication for desktops, PrintWindow
// for windows) and the scene is composited onto a fixed-size canvas (the
// stream output size, 16:9 by default). The canvas is what gets streamed:
// canvasReady emits the composed frame for the ffmpeg rawvideo pipe.
//
// The canvas is interactive, OBS-style: click a source to select it, drag to
// move it, drag its bottom-right corner to resize it.
#include "FFmpegInputDevice.h"
#include "../widgets/FileDropZone.h"

#include <QHash>
#include <QImage>
#include <QMovie>
#include <QMutex>
#include <QSize>
#include <QTimer>

#include "StreamSource.h"

class FFmpegVideo;

class ScenePreview : public FileDropZone
{
	Q_OBJECT

public:
	ScenePreview(QWidget* parent = nullptr);
	~ScenePreview() override;

	QSize canvasSize() const { return m_canvasSize; }
	void setCanvasSize(const QSize& size);
	void setSources(const QList<StreamSource>& sources);
	void setSelectedIndex(int index); // -1 clears
	void stopAll();

	void paintEvent(QPaintEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

signals:
	void canvasReady(const QImage& canvas);
	void sourceSelected(int index);
	void sceneChanged(const QList<StreamSource>& sources);

private:
	struct RenderSlot
	{
		quint64 id = 0;
		StreamSource source;
		FFmpegInputDevice device;             // stable address handed to the decoder
		FFmpegVideo* video = nullptr;         // camera/file decode thread
		QThread* capture = nullptr;           // desktop/window capture thread
		QImage frame;
		bool aspectAdjusted = false;
	};

	RenderSlot* findSlot(quint64 id);
	void startSlot(RenderSlot* slot);
	void ensureDefaultRects();
	void composeCanvas();
	QRectF canvasRectInWidget() const;
	QRectF sourceRectInWidget(const QRect& rect) const;
	QPointF toCanvasCoords(const QPointF& widgetPos) const;
	int hitTest(const QPointF& canvasPos) const;

	QList<RenderSlot*> m_slots;               // owned; index == z-order
	QSize m_canvasSize{ 1280, 720 };
	QImage m_canvas;
	QMutex m_frameMutex;
	QMovie m_movie{ ":/main/res/splash.gif" };
	QTimer m_composeTimer{ this };

	// Canvas interaction state.
	int m_selectedIndex = -1;
	enum class DragMode { None, Move, Resize };
	DragMode m_dragMode = DragMode::None;
	QPointF m_dragGrabOffset;
};
