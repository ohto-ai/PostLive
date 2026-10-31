#pragma once
// In-process HLS encoder (the OBS architecture): the composited canvas is
// encoded with libavcodec (openh264 from the bundled FFmpeg runtime) and
// written with libavformat's hls muxer directly into the hls directory.
// Timestamps are generated here and start at zero, so segments land on disk
// in real time — no stdin pipes, no external-scheduler stalls.
#include <QImage>
#include <QMutex>
#include <QSize>
#include <QThread>

#include "StreamSource.h"

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct SwsContext;

class SceneEncoder : public QThread
{
	Q_OBJECT

public:
	SceneEncoder(QObject* parent = nullptr);
	~SceneEncoder() override;

	void startHls(const QString& hlsDir, const QSize& canvasSize, int fps,
		const QString& encoder, int bitrateKbps);
	void run() override;
	void stop();

public slots:
	// Latest composited canvas from ScenePreview (scaled to the canvas size).
	void onCanvasReady(const QImage& canvas);

private:
	void shutdown();
	bool openOutput();

	QString m_hlsDir;
	QSize m_canvasSize;
	int m_fps = 30;
	QString m_encoderName;
	int m_bitrateKbps = 2500;

	QMutex m_frameMutex;
	QImage m_latestCanvas;   // ARGB32, canvasSize
	bool m_hasFrame = false;

	AVFormatContext* m_formatContext = nullptr;
	AVCodecContext* m_codecContext = nullptr;
	AVFrame* m_frame = nullptr;
	SwsContext* m_swsContext = nullptr;
	int m_videoStreamIndex = -1;
	qint64 m_frameIndex = 0;
};
