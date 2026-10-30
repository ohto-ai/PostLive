#if defined(_MSC_VER) && (_MSC_VER >= 1600)
# pragma execution_character_set("utf-8")
#endif

#include "LivePlatform.h"
#include "LiveLoginSettingDialog.h"
#include "../core/Logging.h"
#include "../platform/WindowList.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QSet>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QUrl>

namespace
{
	QString streamSetting(const char* key, const char* fallback)
	{
		return QString::fromStdString(
			thatboy::storage::config["stream"].value(key, std::string(fallback)));
	}
}

LivePlatform::LivePlatform(QWidget* parent)
	: DragableMainWindow(parent)
{
	ui.setupUi(this);

	sourceGroup.addButton(ui.sourceCardCamera, 0);
	sourceGroup.addButton(ui.sourceCardDesktop, 1);
	sourceGroup.addButton(ui.sourceCardWindow, 2);
	sourceGroup.addButton(ui.sourceCardFile, 3);
	sourceGroup.setExclusive(true);

	thatboy::storage::config["widget_info"]["platform"].get<thatboy::WidgetConfigInfo>().config(*this);

	refreshDevices();
	applySourceMode();
	ui.cameraViewLayout->addWidget(&previewWidget);

	auto mSysTrayIcon = new QSystemTrayIcon(this);
	mSysTrayIcon->setIcon(QIcon(":/LiveLoginDialog/res/live.ico"));
	mSysTrayIcon->setToolTip(QObject::trUtf8("PostLive"));

	auto m_menu = new QMenu(this);
	auto actionShowMainWindow = new QAction(QIcon(":/LiveLoginDialog/res/live.ico"), "显示主窗口", m_menu);
	auto actionSettings = new QAction("设置", m_menu);
	auto actionViewWebsite = new QAction(QIcon(":/LiveLoginDialog/res/boy.ico"), "浏览官网", m_menu);
	auto actionViewGithub = new QAction(QIcon(":/LiveLoginDialog/res/github.png"), "项目地址", m_menu);
	auto actionExit = new QAction("退出客户端", m_menu);

	m_menu->addAction(actionShowMainWindow);
	m_menu->addAction(actionSettings);
	m_menu->addAction(actionViewWebsite);
	m_menu->addAction(actionViewGithub);
	m_menu->addAction(actionExit);

	connect(actionShowMainWindow, &QAction::triggered, this, &LivePlatform::showFromTray);
	connect(actionSettings, &QAction::triggered, this, &LivePlatform::openSettings);
	connect(actionViewWebsite, &QAction::triggered, this, [this]
		{
			if (!thatboy::storage::websiteUrl.isEmpty())
				QDesktopServices::openUrl(QUrl(thatboy::storage::websiteUrl));
		});
	connect(actionViewGithub, &QAction::triggered, std::bind(&QDesktopServices::openUrl, QUrl(thatboy::storage::GithubUrl)));
	connect(actionExit, &QAction::triggered, this, [this]
		{
			if (ffmpegProcess.state() != QProcess::NotRunning)
			{
				stopStreaming();
				if (!ffmpegProcess.waitForFinished(3000))
				{
					ffmpegProcess.terminate();
					if (!ffmpegProcess.waitForFinished(2000))
						ffmpegProcess.kill();
				}
			}
			close();
		});

	mSysTrayIcon->setContextMenu(m_menu);
	connect(mSysTrayIcon, &QSystemTrayIcon::activated, [&, actionShowMainWindow](QSystemTrayIcon::ActivationReason reason)
		{
			switch (reason) {
			case QSystemTrayIcon::DoubleClick:
				if (isHidden() || isMinimized())
					showFromTray();
				else
					hideToTray();
				break;
			default:
				break;
			}
		});
	mSysTrayIcon->show();

	connect(this, &BaseMainWindow::closed, [&]
		{
			thatboy::storage::config["widget_info"]["platform"] = thatboy::WidgetConfigInfo(*this);
		});
	setAttribute(Qt::WA_TranslucentBackground);
	connect(ui.closeToolButton, &QToolButton::clicked, this, &LivePlatform::hideToTray);
	connect(ui.minimizeToolButton, &QToolButton::clicked, this, &LivePlatform::showMinimized);
	connect(ui.settingsPushButton, &QPushButton::clicked, this, &LivePlatform::openSettings);

	connect(&ffmpegProcess, &QProcess::started, [this]
		{
			updateStreamControls(true);
			appendStatus(tr("推流中"));
		});
	connect(&ffmpegProcess, &QProcess::readyReadStandardError, [this]
		{
			lastErrorOutput += QString::fromLocal8Bit(ffmpegProcess.readAllStandardError());
			if (lastErrorOutput.size() > 4096)
				lastErrorOutput = lastErrorOutput.right(4096);
		});
	connect(&ffmpegProcess, &QProcess::readyReadStandardOutput, [this]
		{
			// -progress pipe:1 emits key=value lines; surface duration,
			// bitrate and fps in the control bar while streaming.
			const QList<QByteArray> lines = ffmpegProcess.readAllStandardOutput().split('\n');
			QString outTime, bitrate, fps;
			for (const QByteArray& raw : lines)
			{
				const QString line = QString::fromLatin1(raw).trimmed();
				const int eq = line.indexOf('=');
				if (eq <= 0)
					continue;
				const QString key = line.left(eq);
				const QString value = line.mid(eq + 1).trimmed();
				if (key == QLatin1String("out_time"))
					outTime = value;
				else if (key == QLatin1String("bitrate") && value.endsWith(QLatin1String("bits/s")))
					bitrate = value.left(value.size() - 6).trimmed();
				else if (key == QLatin1String("fps"))
					fps = value;
			}
			if (!outTime.isEmpty() || !bitrate.isEmpty() || !fps.isEmpty())
			{
				QString text = QStringLiteral("直播 %1").arg(outTime);
				if (!bitrate.isEmpty())
					text += QStringLiteral(" · %1kbps").arg(bitrate);
				if (!fps.isEmpty())
					text += QStringLiteral(" · %1fps").arg(fps);
				ui.progressLabel->setText(text);
			}
		});
	connect(&ffmpegProcess, &QProcess::errorOccurred, [this](QProcess::ProcessError error)
		{
			if (error != QProcess::FailedToStart)
				return;
			updateStreamControls(false);
			appendStatus(tr("FFmpeg 启动失败"));
			if (!errorNotified)
			{
				errorNotified = true;
				qCCritical(lcStream) << "FFmpeg failed to start; expected at"
					<< ffmpegPath << "or on PATH";
			}
			startPreview();
		});
	connect(&ffmpegProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
		[this](int code, QProcess::ExitStatus status)
		{
			updateStreamControls(false);
			const bool wasStopRequested = stopRequested;
			stopRequested = false;

			const QString errorText = lastErrorOutput.trimmed();
			lastErrorOutput.clear();
			ui.progressLabel->clear();

			if (!wasStopRequested && (code != 0 || status != QProcess::NormalExit))
			{
				appendStatus(tr("推流异常结束"));
				if (!errorNotified)
				{
					errorNotified = true;
					qCWarning(lcStream) << "FFmpeg exited abnormally with code" << code
						<< "stderr tail:" << errorText;
				}
			}
			else
			{
				appendStatus(tr("推流已停止"));
			}

			startPreview();
		});

	connect(ui.startPushButton, &QPushButton::clicked, this, &LivePlatform::startStreaming);
	connect(ui.stopPushButton, &QPushButton::clicked, this, &LivePlatform::stopStreaming);

	// Source cards: switch source, show the matching controls, restart preview.
	connect(&sourceGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, [this](int)
		{
			applySourceMode();
			startPreview();
		});
	connect(ui.windowComboBox, &WindowComboBox::aboutToShowPopup, this, &LivePlatform::refreshWindowList);
	connect(ui.windowComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int)
		{
			startPreview();
		});
	connect(ui.cameraComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int)
		{
			startPreview();
		});
	connect(ui.selectFilePushButton, &QPushButton::clicked, this, [this]
		{
			const QString file = QFileDialog::getOpenFileName(this, tr("选择推流文件"), QString(),
				tr("媒体文件 (*)"));
			if (file.isEmpty())
				return;
			localFilePath = file;
			ui.fileLabel->setText(QFileInfo(file).fileName());
			ui.fileLabel->setToolTip(file);
			startPreview();
		});

	connect(&deviceRefreshTimer, &QTimer::timeout, this, &LivePlatform::refreshDevices);
	deviceRefreshTimer.start(3000);

	startPreview();
}

LivePlatform::~LivePlatform()
{
	if (ffmpegProcess.state() != QProcess::NotRunning)
	{
		ffmpegProcess.kill();
		ffmpegProcess.waitForFinished(1000);
	}
}

void LivePlatform::openSettings()
{
	auto* dialog = new LiveLoginSettingDialog(this);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->show();
	dialog->raise();
	dialog->activateWindow();
}

void LivePlatform::changeEvent(QEvent* event)
{
	if (event->type() == QEvent::WindowStateChange)
	{
		if (isMinimized())
			stopPreview();
		else if (isVisible())
			startPreview();
	}
	DragableMainWindow::changeEvent(event);
}

void LivePlatform::showFromTray()
{
	if (isMinimized())
		showNormal();
	show();
	raise();
	activateWindow();
	startPreview();
}

void LivePlatform::hideToTray()
{
	hide();
	stopPreview();
}

CaptureSource LivePlatform::currentSource() const
{
	switch (sourceGroup.checkedId())
	{
	case 1: return CaptureSource::Desktop;
	case 2: return CaptureSource::Window;
	case 3: return CaptureSource::LocalFile;
	default: return CaptureSource::Camera;
	}
}

QString LivePlatform::cameraDeviceName() const
{
	const int index = ui.cameraComboBox->currentIndex();
	return index >= 0 ? ui.cameraComboBox->itemData(index).toString() : QString();
}

void LivePlatform::applySourceMode()
{
	const CaptureSource source = currentSource();
	ui.cameraLabel->setVisible(source == CaptureSource::Camera);
	ui.cameraComboBox->setVisible(source == CaptureSource::Camera);
	ui.microphoneLabel->setVisible(source != CaptureSource::LocalFile);
	ui.microphoneComboBox->setVisible(source != CaptureSource::LocalFile);
	ui.windowComboBox->setVisible(source == CaptureSource::Window);
	ui.selectFilePushButton->setVisible(source == CaptureSource::LocalFile);
	ui.fileLabel->setVisible(source == CaptureSource::LocalFile);
}

void LivePlatform::startPreview()
{
	if (ffmpegProcess.state() == QProcess::Running)
		return;

	switch (currentSource())
	{
	case CaptureSource::Camera:
	{
		const QString name = cameraDeviceName();
		if (name.isEmpty())
			return;
		bool found = false;
		for (const FFmpegInputDevice* device : FFmpegInputDevice::filterVideoDevices())
		{
			if (QString::fromStdString(device->device_name) == name)
			{
				currentDevice = *device;
				found = true;
				break;
			}
		}
		if (!found)
			return;
		break;
	}
	case CaptureSource::Desktop:
		currentDevice = FFmpegInputDevice::make_gdi_device();
		break;
	case CaptureSource::Window:
	{
		const QString title = ui.windowComboBox->currentText();
		if (title.isEmpty())
			return;
		currentDevice = FFmpegInputDevice::make_gdi_device(title.toStdString());
		break;
	}
	case CaptureSource::LocalFile:
		if (localFilePath.isEmpty() || !QFileInfo::exists(localFilePath))
			return;
		currentDevice = FFmpegInputDevice::make_local_file_device(localFilePath.toStdString());
		break;
	}

	if (!previewWidget.setInputDevice(&currentDevice))
		return;
	previewWidget.play();
}

void LivePlatform::stopPreview()
{
	previewWidget.stop();
}

void LivePlatform::startStreaming()
{
	if (ffmpegProcess.state() != QProcess::NotRunning)
		return;

	StreamSettings settings;
	settings.source = currentSource();
	settings.server = streamSetting("server", "rtmp://rtmp.ohtoai.top");
	settings.cameraName = cameraDeviceName();
	settings.microphoneName = ui.microphoneComboBox->currentText();
	settings.windowTitle = ui.windowComboBox->currentText();
	settings.filePath = localFilePath;
	settings.size = streamSetting("size", "1280x720");
	settings.encoder = streamSetting("encoder", "Auto");
	settings.bitrate = streamSetting("bitrate", "Auto");
	settings.fps = streamSetting("fps", "Auto");
	settings.watermark = thatboy::storage::config["stream"].value("watermark", true);
	settings.watermarkFile = logoImage;

	QString error;
	const QStringList arguments = buildStreamArgs(settings, &error);
	if (arguments.isEmpty())
	{
		appendStatus(tr("无法开始推流：%1").arg(error));
		qCWarning(lcStream) << "stream start aborted:" << error;
		return;
	}
	if (settings.watermark && !QFileInfo::exists(settings.watermarkFile))
	{
		appendStatus(tr("找不到水印文件，本次推流不叠加水印"));
		qCInfo(lcStream) << "watermark file missing, streaming without it:" << settings.watermarkFile;
	}

	stopPreview();
	stopRequested = false;
	errorNotified = false;
	lastErrorOutput.clear();
	ui.progressLabel->clear();
	ffmpegProcess.readAllStandardError();
	appendStatus(tr("正在启动 FFmpeg..."));
	qCInfo(lcStream) << "starting FFmpeg:" << ffmpegPath << arguments.join(QLatin1Char(' '));
	ffmpegProcess.start(ffmpegPath, arguments);
}

void LivePlatform::stopStreaming()
{
	if (ffmpegProcess.state() == QProcess::NotRunning)
		return;

	stopRequested = true;
	appendStatus(tr("正在停止..."));
	ffmpegProcess.write("q");
	ffmpegProcess.closeWriteChannel();

	QTimer::singleShot(3000, this, [this]
		{
			if (ffmpegProcess.state() == QProcess::NotRunning)
				return;
			ffmpegProcess.terminate();
			QTimer::singleShot(2000, this, [this]
				{
					if (ffmpegProcess.state() != QProcess::NotRunning)
						ffmpegProcess.kill();
				});
		});
}

void LivePlatform::updateStreamControls(bool streaming)
{
	ui.sourceCardCamera->setEnabled(!streaming);
	ui.sourceCardDesktop->setEnabled(!streaming);
	ui.sourceCardWindow->setEnabled(!streaming);
	ui.sourceCardFile->setEnabled(!streaming);
	ui.cameraComboBox->setEnabled(!streaming);
	ui.microphoneComboBox->setEnabled(!streaming);
	ui.windowComboBox->setEnabled(!streaming);
	ui.selectFilePushButton->setEnabled(!streaming);
	ui.startPushButton->setVisible(!streaming);
	ui.stopPushButton->setVisible(streaming);
}

void LivePlatform::appendStatus(const QString& text)
{
	ui.statusLabel->setText(text);
	ui.statusLabel->setToolTip(text);
}

void LivePlatform::refreshDevices()
{
	if (ffmpegProcess.state() == QProcess::Running)
		return;

	FFmpegInputDevice::updateInputDeviceList();

	const auto cameras = FFmpegInputDevice::filterVideoDevices();
	QStringList cameraNames;
	for (const auto& camera : cameras)
		cameraNames << QString::fromStdString(camera->device_description);

	QStringList currentCameras;
	for (int i = 0; i < ui.cameraComboBox->count(); ++i)
		currentCameras << ui.cameraComboBox->itemText(i);

	if (cameraNames != currentCameras)
	{
		const QString selected = ui.cameraComboBox->currentText();
		{
			const QSignalBlocker blocker(ui.cameraComboBox);
			ui.cameraComboBox->clear();
			for (const auto& camera : cameras)
				ui.cameraComboBox->addItem(QString::fromStdString(camera->device_description),
					QString::fromStdString(camera->device_name));

			int index = ui.cameraComboBox->findText(selected);
			if (index < 0)
				index = 0;
			if (index < ui.cameraComboBox->count())
				ui.cameraComboBox->setCurrentIndex(index);
		}
		if (isVisible() && !isMinimized() && currentSource() == CaptureSource::Camera)
			startPreview();
	}

	QStringList microphoneNames;
	for (const auto& microphone : FFmpegInputDevice::filterAudioDevices())
		microphoneNames << QString::fromStdString(microphone->device_name);
	microphoneNames << "virtual-audio-capturer";

	QStringList currentMicrophones;
	for (int i = 0; i < ui.microphoneComboBox->count(); ++i)
		currentMicrophones << ui.microphoneComboBox->itemText(i);

	if (microphoneNames != currentMicrophones)
	{
		const QString selected = ui.microphoneComboBox->currentText();
		{
			const QSignalBlocker blocker(ui.microphoneComboBox);
			ui.microphoneComboBox->clear();
			ui.microphoneComboBox->addItems(microphoneNames);

			int index = ui.microphoneComboBox->findText(selected);
			if (index < 0)
				index = 0;
			if (index < ui.microphoneComboBox->count())
				ui.microphoneComboBox->setCurrentIndex(index);
		}
	}

	if (ui.windowComboBox->isVisible())
		refreshWindowList();
}

void LivePlatform::refreshWindowList()
{
	const QString selected = ui.windowComboBox->currentText();
	const QSignalBlocker blocker(ui.windowComboBox);
	ui.windowComboBox->clear();
	for (const WindowInfo& window : visibleTopLevelWindows())
		ui.windowComboBox->addItem(window.title);
	if (!selected.isEmpty())
	{
		const int index = ui.windowComboBox->findText(selected);
		if (index >= 0)
			ui.windowComboBox->setCurrentIndex(index);
	}
}
