#pragma once
// Decode thread producing RGB32 QImages from an FFmpegInputDevice.
// Ported from PostLive.Re (PostLiveClient/ffmpeg/FFmpegVideo).
#include <QThread>
#include <QImage>
#include <mutex>

class FFmpegVideo : public QThread
{
	Q_OBJECT
public:
	FFmpegVideo(QObject* parent = nullptr);
	~FFmpegVideo() override;

	bool setInputDevice(const struct FFmpegInputDevice* device);
	void run() override;
	void stop();
	void addOption(const QString& key, const QString& value);

signals:
	void postError(int error, const QString& msg);
	void frameReady(const QImage& image);

private:
	void clean();
	bool open();
	void postFFmpegError(int error);

	const struct FFmpegInputDevice* inputDevice = nullptr;
	struct AVFormatContext* inputFormatContext = nullptr;
	struct AVCodecContext* inputVideoCodecContext = nullptr;
	struct AVDictionary* inputOptions = nullptr;
	const struct AVCodec* inputVideoCodec = nullptr;

	int inputVideoStreamIndex = -1;
	std::mutex inputDeviceMutex;
	struct SwsContext* img_ctx = nullptr;
};
