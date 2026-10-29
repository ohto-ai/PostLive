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

LivePlatform::LivePlatform(QWidget* parent)
	: DragableMainWindow(parent)
{
	ui.setupUi(this);

	ui.sourceComboBox->addItem("摄像头");
	ui.sourceComboBox->addItem("整个屏幕");
	ui.sourceComboBox->addItem("窗口");
	ui.sourceComboBox->addItem("本地文件");

	ui.bitrateComboBox->addItems({ "Auto", "1000", "2500", "4000", "6000" });
	ui.frameRateComboBox->addItems({ "Auto", "15", "24", "30", "60" });
	ui.encoderComboBox->addItems({ "Auto", "libx264", "h264_nvenc", "h264_amf", "h264_qsv", "libopenh264" });

	const auto& streamConfig = thatboy::storage::config["stream"];
	auto restoreCombo = [&](QComboBox* combo, const char* key)
	{
		if (streamConfig.contains(key) && streamConfig[key].is_string())
			combo->setCurrentText(QString::fromStdString(streamConfig[key].get<std::string>()));
	};
	restoreCombo(ui.bitrateComboBox, "bitrate");
	restoreCombo(ui.frameRateComboBox, "fps");
	restoreCombo(ui.encoderComboBox, "encoder");
	if (streamConfig.contains("preview") && streamConfig["preview"].is_boolean())
		ui.previewCheckBox->setChecked(streamConfig["preview"].get<bool>());

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
	connect(actionSettings, &QAction::triggered, this, [this]
		{
			auto* dialog = new LiveLoginSettingDialog(this);
			dialog->setAttribute(Qt::WA_DeleteOnClose);
			dialog->show();
			dialog->raise();
			dialog->activateWindow();
		});
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
			auto& config = thatboy::storage::config;
			config["widget_info"]["platform"] = thatboy::WidgetConfigInfo(*this);
			auto& stream = config["stream"];
			stream["bitrate"] = ui.bitrateComboBox->currentText().toStdString();
			stream["fps"] = ui.frameRateComboBox->currentText().toStdString();
			stream["encoder"] = ui.encoderComboBox->currentText().toStdString();
			stream["preview"] = ui.previewCheckBox->isChecked();
		});
	setAttribute(Qt::WA_TranslucentBackground);
	connect(ui.closeToolButton, &QToolButton::clicked, this, &LivePlatform::hideToTray);
	connect(ui.minimizeToolButton, &QToolButton::clicked, this, &LivePlatform::showMinimized);

	connect(ui.previewCheckBox, &QCheckBox::toggled, this, [this](bool checked)
		{
			if (ffmpegProcess.state() == QProcess::Running)
				return;
			if (checked)
				startPreview();
			else
				stopPreview();
		});

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
			if (ui.previewCheckBox->isChecked())
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

			if (ui.previewCheckBox->isChecked())
				startPreview();
		});

	connect(ui.startPushButton, &QPushButton::clicked, this, &LivePlatform::startStreaming);
	connect(ui.stopPushButton, &QPushButton::clicked, this, &LivePlatform::stopStreaming);

	// Source switching: show the matching controls and restart the preview.
	connect(ui.sourceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int)
		{
			applySourceMode();
			if (ui.previewCheckBox->isChecked() && ffmpegProcess.state() != QProcess::Running)
				startPreview();
		});
	connect(ui.windowComboBox, &WindowComboBox::aboutToShowPopup, this, &LivePlatform::refreshWindowList);
	connect(ui.windowComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int)
		{
			if (ui.previewCheckBox->isChecked() && ffmpegProcess.state() != QProcess::Running)
				startPreview();
		});
	connect(ui.selectFilePushButton, &QPushButton::clicked, this, [this]
		{
			const QString file = QFileDialog::getOpenFileName(this, tr("选择推流文件"), QString(),
				tr("媒体文件 (*)"));
			if (file.isEmpty())
				return;
			localFilePath = file;
			appendStatus(tr("已选择文件：%1").arg(file));
			if (ui.previewCheckBox->isChecked() && ffmpegProcess.state() != QProcess::Running)
				startPreview();
		});

	connect(&deviceRefreshTimer, &QTimer::timeout, this, &LivePlatform::refreshDevices);
	deviceRefreshTimer.start(3000);
}

LivePlatform::~LivePlatform()
{
	if (ffmpegProcess.state() != QProcess::NotRunning)
	{
		ffmpegProcess.kill();
		ffmpegProcess.waitForFinished(1000);
	}
}

void LivePlatform::changeEvent(QEvent* event)
{
	if (event->type() == QEvent::WindowStateChange)
	{
		if (isMinimized())
		{
			if (ffmpegProcess.state() != QProcess::Running)
				stopPreview();
		}
		else if (isVisible())
		{
			if (ffmpegProcess.state() != QProcess::Running)
				startPreview();
		}
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
	if (ffmpegProcess.state() != QProcess::Running)
		startPreview();
}

void LivePlatform::hideToTray()
{
	hide();
	if (ffmpegProcess.state() != QProcess::Running)
		stopPreview();
}

CaptureSource LivePlatform::currentSource() const
{
	switch (ui.sourceComboBox->currentIndex())
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
}

void LivePlatform::startPreview()
{
	if (ffmpegProcess.state() == QProcess::Running || !ui.previewCheckBox->isChecked())
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
	settings.server = ui.streamServerLineEdit->text().trimmed();
	settings.cameraName = cameraDeviceName();
	settings.microphoneName = ui.microphoneComboBox->currentText();
	settings.windowTitle = ui.windowComboBox->currentText();
	settings.filePath = localFilePath;
	settings.size = ui.sizeComboBox->currentText();
	settings.encoder = ui.encoderComboBox->currentText();
	settings.bitrate = ui.bitrateComboBox->currentText();
	settings.fps = ui.frameRateComboBox->currentText();
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
	ui.sourceComboBox->setEnabled(!streaming);
	ui.windowComboBox->setEnabled(!streaming);
	ui.selectFilePushButton->setEnabled(!streaming);
	ui.cameraComboBox->setEnabled(!streaming);
	ui.microphoneComboBox->setEnabled(!streaming);
	ui.streamServerLineEdit->setEnabled(!streaming);
	ui.sizeComboBox->setEnabled(!streaming);
	ui.bitrateComboBox->setEnabled(!streaming);
	ui.frameRateComboBox->setEnabled(!streaming);
	ui.encoderComboBox->setEnabled(!streaming);
	ui.previewCheckBox->setEnabled(!streaming);
	ui.startPushButton->setEnabled(!streaming);
	ui.stopPushButton->setEnabled(streaming);
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
		if (ui.previewCheckBox->isChecked() && ffmpegProcess.state() != QProcess::Running
			&& isVisible() && !isMinimized() && currentSource() == CaptureSource::Camera)
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
