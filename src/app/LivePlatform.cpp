#if defined(_MSC_VER) && (_MSC_VER >= 1600)
# pragma execution_character_set("utf-8")
#endif

#include "LivePlatform.h"
#include "LiveLoginSettingDialog.h"
#include "../core/Logging.h"

#include <future>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QEvent>
#include <QFileInfo>
#include <QMenu>
#include <QSet>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QUrl>
#include <QAudioDeviceInfo>

Q_DECLARE_METATYPE(QCameraInfo)

LivePlatform::LivePlatform(QWidget* parent)
	: DragableMainWindow(parent)
{
	ui.setupUi(this);

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

	auto cameraList = QCameraInfo::availableCameras();

	for (auto& cameraInfo : cameraList)
		ui.cameraComboBox->addItem(cameraInfo.description(), QVariant::fromValue(cameraInfo));
	if (!cameraList.isEmpty())
		ui.cameraComboBox->setCurrentText(QCameraInfo::defaultCamera().description());

	auto microphoneList = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
	QSet<QString> microphoneNameList;
	for (auto& microphoneInfo : microphoneList)
		microphoneNameList.insert(microphoneInfo.deviceName());

	microphoneNameList.insert("virtual-audio-capturer");

	for (auto& microphoneName : microphoneNameList)
		ui.microphoneComboBox->addItem(microphoneName);
	if (!microphoneList.isEmpty())
		ui.microphoneComboBox->setCurrentText(QAudioDeviceInfo::defaultInputDevice().deviceName());

	ui.sizeComboBox->addItems({
		"640x480", "800x600", "1024x768", "1280x720", "1280x800", "1400x1050", "1440x900", "1600x1200", "1920x1080", "1920x1200"
		});
	ui.sizeComboBox->setCurrentText("1280x720");

	ui.cameraViewLayout->addWidget(&cameraViewfinder);

	connect(ui.cameraComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int)
		{
			setViewCamera(qvariant_cast<QCameraInfo>(ui.cameraComboBox->currentData()));
			if (ui.previewCheckBox->isChecked() && ffmpegProcess.state() != QProcess::Running)
				startPreview();
		});

	setViewCamera(QCameraInfo::defaultCamera());
	if (ui.previewCheckBox->isChecked())
		startPreview();

	connect(&deviceRefreshTimer, &QTimer::timeout, this, &LivePlatform::refreshDevices);
	deviceRefreshTimer.start(3000);
}

void LivePlatform::setViewCamera(const QCameraInfo& cameraInfo)
{
	delete viewCamera;
	viewCamera = new QCamera(cameraInfo, this);
	viewCamera->setViewfinder(&cameraViewfinder);
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

void LivePlatform::startPreview()
{
	if (viewCamera && ui.previewCheckBox->isChecked())
		viewCamera->start();
}

void LivePlatform::stopPreview()
{
	if (viewCamera)
		viewCamera->stop();
}

void LivePlatform::startStreaming()
{
	if (ffmpegProcess.state() != QProcess::NotRunning)
		return;

	const QString server = ui.streamServerLineEdit->text().trimmed();
	const QUrl serverUrl(server);
	if (server.isEmpty() || !serverUrl.isValid()
		|| serverUrl.scheme().isEmpty() || serverUrl.host().isEmpty())
	{
		appendStatus(tr("无效的服务器地址"));
		qCWarning(lcStream) << "stream start aborted: invalid server URL" << server;
		return;
	}

	const QString camera = ui.cameraComboBox->currentText();
	const QString microphone = ui.microphoneComboBox->currentText();
	if (camera.isEmpty())
	{
		appendStatus(tr("没有可用的摄像头"));
		qCWarning(lcStream) << "stream start aborted: no camera";
		return;
	}
	if (microphone.isEmpty())
	{
		appendStatus(tr("没有可用的麦克风"));
		qCWarning(lcStream) << "stream start aborted: no microphone";
		return;
	}

	QString ffmpegExecutable = ffmpegPath;
	if (!QFileInfo::exists(ffmpegExecutable))
		ffmpegExecutable = QStandardPaths::findExecutable("ffmpeg.exe");
	if (ffmpegExecutable.isEmpty())
	{
		appendStatus(tr("找不到 FFmpeg，请放到程序目录或 PATH"));
		qCWarning(lcStream) << "stream start aborted: ffmpeg.exe not found";
		return;
	}

	QString size = ui.sizeComboBox->currentText();
	size.replace(QChar(0x00D7), QLatin1Char('x'));

	QString url = server;
	while (url.endsWith(QLatin1Char('/')))
		url.chop(1);
	url += "/livehime";

	const bool watermarkEnabled = thatboy::storage::config["stream"].value("watermark", true);
	const bool useWatermark = watermarkEnabled && QFileInfo::exists(logoImage);
	if (watermarkEnabled && !useWatermark)
	{
		appendStatus(tr("找不到水印文件，本次推流不叠加水印"));
		qCInfo(lcStream) << "watermark file missing, streaming without it:" << logoImage;
	}

	QStringList arguments;
	arguments << "-hide_banner" << "-loglevel" << "warning" << "-nostats";
	arguments << "-f" << "dshow" << "-i" << QString("video=%1").arg(camera);
	if (useWatermark)
		arguments << "-i" << logoImage;
	arguments << "-f" << "dshow" << "-i" << QString("audio=%1").arg(microphone);
	if (useWatermark)
		arguments << "-filter_complex" << "overlay=5:5";
	arguments << "-f" << videoFormt;
	if (ui.encoderComboBox->currentText() != "Auto")
		arguments << "-c:v" << ui.encoderComboBox->currentText();
	if (ui.bitrateComboBox->currentText() != "Auto")
		arguments << "-b:v" << ui.bitrateComboBox->currentText() + "k";
	if (ui.frameRateComboBox->currentText() != "Auto")
		arguments << "-r" << ui.frameRateComboBox->currentText();
	arguments << "-pix_fmt" << "yuv420p";
	arguments << "-s" << size << url;

	stopPreview();
	stopRequested = false;
	errorNotified = false;
	lastErrorOutput.clear();
	ffmpegProcess.readAllStandardError();
	appendStatus(tr("正在启动 FFmpeg..."));
	qCInfo(lcStream) << "starting FFmpeg:" << ffmpegExecutable << arguments.join(QLatin1Char(' '));
	ffmpegProcess.start(ffmpegExecutable, arguments);
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

	const auto cameras = QCameraInfo::availableCameras();
	QStringList cameraNames;
	for (const auto& camera : cameras)
		cameraNames << camera.description();

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
				ui.cameraComboBox->addItem(camera.description(), QVariant::fromValue(camera));

			int index = ui.cameraComboBox->findText(selected);
			if (index < 0)
				index = 0;
			if (index < ui.cameraComboBox->count())
				ui.cameraComboBox->setCurrentIndex(index);
		}

		setViewCamera(ui.cameraComboBox->count() > 0
			? qvariant_cast<QCameraInfo>(ui.cameraComboBox->currentData())
			: QCameraInfo());
		if (isVisible() && !isMinimized() && ui.previewCheckBox->isChecked())
			startPreview();
	}

	QSet<QString> microphoneSet;
	const auto microphones = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
	for (const auto& microphone : microphones)
		microphoneSet.insert(microphone.deviceName());
	microphoneSet.insert("virtual-audio-capturer");

	QSet<QString> currentMicrophoneSet;
	for (int i = 0; i < ui.microphoneComboBox->count(); ++i)
		currentMicrophoneSet.insert(ui.microphoneComboBox->itemText(i));

	if (microphoneSet != currentMicrophoneSet)
	{
		const QString selected = ui.microphoneComboBox->currentText();
		QStringList names = microphoneSet.values();
		names.sort(Qt::CaseInsensitive);
		ui.microphoneComboBox->clear();
		ui.microphoneComboBox->addItems(names);

		int index = ui.microphoneComboBox->findText(selected);
		if (index < 0 && !microphones.isEmpty())
			index = ui.microphoneComboBox->findText(QAudioDeviceInfo::defaultInputDevice().deviceName());
		if (index < 0)
			index = 0;
		if (index < ui.microphoneComboBox->count())
			ui.microphoneComboBox->setCurrentIndex(index);
	}
}
