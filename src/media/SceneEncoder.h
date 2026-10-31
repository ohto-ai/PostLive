#pragma once
// In-process stream encoder (the OBS architecture): the composited canvas is
// encoded with libavcodec (openh264 from the bundled FFmpeg runtime) and
// muxed with libavformat — either as an HLS playlist directory (local
// browser viewing) or pushed as RTMP/FLV. Timestamps are generated here and
// start at zero, so output lands in real time — no stdin pipes, no
// external-scheduler stalls, and the preview keeps running while live.
//
// Audio: the microphone is captured in-process (MicrophoneCapture, raw PCM)
// and encoded to AAC on a second stream. Because both streams are stamped by
// us, the dshow wallclock offset never reaches the output. If the microphone
// races ahead of the video clock (encode stall under load), excess samples
// are dropped to keep the muxer flowing.
#include <QByteArray>
#include <QImage>
#include <QMutex>
#include <QAtomicInteger>
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
	enum class OutputMode
	{
		LocalHls, // muxer writes an HLS playlist directory
		Rtmp,     // muxer pushes FLV over rtmp://
	};

	SceneEncoder(QObject* parent = nullptr);
	~SceneEncoder() override;

	// target = the HLS directory (LocalHls) or the rtmp:// base URL (Rtmp);
	// "/livehime" is appended for RTMP.
	void setMuted(bool muted) { m_muted.store(muted); }

	void startStream(OutputMode mode, const QString& target, const QSize& canvasSize, int fps,
		const QString& encoder, int bitrateKbps, bool withAudio,
		const QString& microphoneName);
	void run() override;
	void stop();

public slots:
	// Latest composited canvas from ScenePreview (scaled to the canvas size).
	void onCanvasReady(const QImage& canvas);
	// Interleaved fltp stereo samples from MicrophoneCapture.
	void onAudioReady(const QByteArray& samplesFltp, int sampleCount);
	void onAudioLevel(float level);

signals:
	void errorOccurred(const QString& message);
	void streamStarted();
	void streamStopped();
	void streamStats(int seconds, int kbps);
	void audioLevel(float level);

private:
	struct RenderSlot; // ScenePreview owns the slots; keep forward decls local
	void shutdown();
	bool openOutput();
	bool openAudioStream();
	void drainAudioFifo();
	void flushEncoders();

	OutputMode m_mode = OutputMode::LocalHls;
	QString m_target;
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

	QAtomicInteger<bool> m_muted{ false };
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
	qint64 m_encodedBytes = 0;
};
