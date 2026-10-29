#include "FFmpegWidget.h"

#include "FFmpegInputDevice.h"
#include "FFmpegVideo.h"
#include "../core/Logging.h"

#include <QDateTime>
#include <QPainter>
#include <QStatusBar>
#include <QTimer>

FFmpegWidget::FFmpegWidget(QWidget* parent)
	: FileDropZone(parent)
{
	ffmpegVideo = new FFmpegVideo(this);

	// 黑色背景
	QPalette pal = palette();
	pal.setColor(QPalette::Window, Qt::black);
	setAutoFillBackground(true);
	setPalette(pal);

	connect(ffmpegVideo, &FFmpegVideo::frameReady, this, &FFmpegWidget::setFrame, Qt::QueuedConnection);
	connect(ffmpegVideo, &FFmpegVideo::postError, this,
		[this](int ec, const QString& error)
		{
			qCWarning(lcStream) << "preview error:" << ec << error;
			ffmpegVideoErrorCode = ec;
			ffmpegVideoErrorString = error;
			clearFrame();
			update();
			QTimer::singleShot(3000, this, [this]
				{
					ffmpegVideoErrorCode = 0;
					ffmpegVideoErrorString.clear();
					updateStatusBar();
					update();
				});
		}, Qt::QueuedConnection);

	// Dropping a media file switches this widget to local-file playback.
	connect(this, &FileDropZone::onFileDropped, this,
		[this](const QStringList& paths)
		{
			if (paths.isEmpty())
				return;
			static FFmpegInputDevice device; // outlives the decode thread
			device = FFmpegInputDevice::make_local_file_device(paths.front().toStdString());
			stop();
			setInputDevice(&device);
			play();
		});

	// Loading animation while no frame is available (resource added in the
	// UI commit; QMovie silently stays empty when it is missing).
	if (movie.isValid())
	{
		connect(&movie, &QMovie::frameChanged, this,
			[this]
			{
				frameImage = movie.currentImage().scaled(width(), height(), Qt::KeepAspectRatio);
				update();
			});
	}
	clearFrame();
}

FFmpegWidget::~FFmpegWidget()
{
	delete ffmpegVideo;
}

bool FFmpegWidget::setInputDevice(const FFmpegInputDevice* device)
{
	return ffmpegVideo->setInputDevice(device);
}

void FFmpegWidget::play()
{
	ffmpegVideo->start();
}

void FFmpegWidget::stop()
{
	ffmpegVideo->stop();
	clearFrame();
}

void FFmpegWidget::addOption(const QString& key, const QString& value)
{
	ffmpegVideo->addOption(key, value);
}

void FFmpegWidget::setStatusStatusBar(QStatusBar* statusBar)
{
	statusStatusBar = statusBar;
}

void FFmpegWidget::setFrame(const QImage& frame)
{
	// decode FPS over a one-second window
	static int frameCount = 0;
	static qint64 lastTime = QDateTime::currentMSecsSinceEpoch();
	constexpr qint64 CountDuration = 1000;
	frameCount++;
	const qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
	if (currentTime - lastTime >= CountDuration)
	{
		decodeFPS = static_cast<qreal>(frameCount * CountDuration) / (currentTime - lastTime);
		frameCount = 0;
		lastTime = currentTime;
	}

	frameAvailable = true;
	frameImage = frame.scaled(width(), height(), Qt::KeepAspectRatio);
	movie.stop();
	updateStatusBar();
	update();
}

void FFmpegWidget::clearFrame()
{
	frameAvailable = false;
	if (movie.isValid())
		movie.start();
	updateStatusBar();
	update();
}

void FFmpegWidget::updateStatusBar()
{
	if (statusStatusBar == nullptr)
		return;

	QString status;
	if (frameAvailable)
	{
		if (option_displayFPS)
			status += QStringLiteral("Decode FPS: %1 ").arg(decodeFPS, 0, 'f', 1);
		status += QStringLiteral(" | Size: %1x%2").arg(frameImage.width()).arg(frameImage.height());
		if (ffmpegVideo->inputDevice != nullptr)
		{
			QString url = QString::fromStdString(ffmpegVideo->inputDevice->streamUrls[AVMEDIA_TYPE_VIDEO]);
			if (url.size() > 50)
				url = QStringLiteral("%1...%2").arg(url.left(39), url.right(8));
			status += QStringLiteral(" | URL: %1").arg(url);
		}
	}
	if (ffmpegVideoErrorCode != 0)
		status += QStringLiteral(" | Error: %1").arg(ffmpegVideoErrorString);
	statusStatusBar->showMessage(status);
}

void FFmpegWidget::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);

	const int x = (width() - frameImage.width()) / 2;
	const int y = (height() - frameImage.height()) / 2;
	painter.drawImage(x, y, frameImage);

	if (!frameAvailable)
	{
		QFont font = painter.font();
		font.setPointSize(14);
		painter.setFont(font);
		painter.setPen(Qt::white);
		painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("PostLive"));
	}
}
