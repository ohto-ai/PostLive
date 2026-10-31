#pragma once
// In-process HLS encoder (the OBS architecture): the composited canvas is
// encoded with libavcodec (openh264 from the bundled FFmpeg runtime) and
// written with libavformat's hls muxer directly into the hls directory.
// Timestamps are generated here and start at zero, so segments land on disk
// in real time — no stdin pipes, no external-scheduler stalls.
//
// Audio: the microphone is captured in-process (MicrophoneCapture, raw PCM)
// and encoded to AAC on a second stream. Because both streams are stamped
// by us, the dshow wallclock offset never reaches the output.
#include <QByteArray>
#include <QImage>
#include <QMutex>
#include <QSize>
#include <QThread>

#include "StreamSource.h"

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVAudioFifo;
struct SwsContext;
class MicrophoneCapture;

class SceneEncoder : public QThread
{
	Q_OBJECT

public:
	SceneEncoder(QObject* parent = nullptr);
	~SceneEncoder() override;

	void startHls(const QString& hlsDir, const QSize& canvasSize, int fps,
		const QString& encoder, int bitrateKbps, bool withAudio,
		const QString& microphoneName);
	void run() override;
	void stop();

public slots:
	// Latest composited canvas from ScenePreview (scaled to the canvas size).
	void onCanvasReady(const QImage& canvas);
	// Interleaved fltp stereo samples from MicrophoneCapture.
	void onAudioReady(const QByteArray& samplesFltp, int sampleCount);

private:
	void shutdown();
	bool openOutput();
	bool openAudioStream();
	void drainAudioFifo();
	void flushEncoders();

	struct RenderSlot; // ScenePreview owns the slots; keep forward decls local
	QString m_hlsDir;
	QSize m_canvasSize;
	int m_fps = 30;
	QString m_encoderName;
	int m_bitrateKbps = 2500;
	bool m_withAudio = false;
	QString m_microphoneName;

	QMutex m_frameMutex;
	QImage m_latestCanvas;   // ARGB32, canvasSize

	QMutex m_audioMutex;
	AVAudioFifo* m_audioFifo = nullptr;
	bool m_hasAudio = false;

	MicrophoneCapture* m_micCapture = nullptr;
	AVFormatContext* m_formatContext = nullptr;
	AVCodecContext* m_videoCodecContext = nullptr;
	AVCodecContext* m_audioCodecContext = nullptr;
	AVFrame* m_videoFrame = nullptr;
	AVFrame* m_audioFrame = nullptr;
	SwsContext* m_swsContext = nullptr;
	int m_videoStreamIndex = -1;
	int m_audioStreamIndex = -1;
	qint64 m_frameIndex = 0;
	qint64 m_audioSampleIndex = 0;
};
