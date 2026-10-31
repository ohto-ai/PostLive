#pragma once
// In-process microphone capture through libav's dshow demuxer: opens the
// audio device inside our own process, reads raw PCM packets, resamples to
// fltp/48kHz/stereo and hands the samples to the HLS encoder. Input-side
// dshow timestamps are discarded — the encoder stamps by sample count, so
// the capture-graph clock offset (uptime-based) never reaches the output.
#include <QThread>
#include <QString>

struct SwrContext;

class MicrophoneCapture : public QThread
{
	Q_OBJECT

public:
	MicrophoneCapture(QString deviceName, QObject* parent = nullptr);
	void run() override;
	void stop();

signals:
	// Interleaved fltp samples (48 kHz stereo), sampleCount = frames.
	void audioReady(const QByteArray& samplesFltp, int sampleCount);
	void audioLevel(float level); // 0..1 peak of the emitted chunk

private:
	QString m_deviceName;
};
