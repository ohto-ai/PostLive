#if defined(_MSC_VER) && (_MSC_VER >= 1600)
# pragma execution_character_set("utf-8")
#endif

#include "LivePlatform.h"
#include "FFmpegInputDevice.h"
#include "LiveLoginSettingDialog.h"
#include "../core/Logging.h"
#include "../platform/WindowList.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QSignalBlocker>
#include <QUrl>
#include <QVBoxLayout>

namespace
{
	QString streamSetting(const char* key, const char* fallback)
	{
		return QString::fromStdString(
			thatboy::storage::config["stream"].value(key, std::string(fallback)));
	}

	QList<StreamSource> loadScene(const nlohmann::json& config)
	{
		QList<StreamSource> scene;
		if (!config.contains("scene") || !config["scene"].is_object()
			|| !config["scene"].contains("sources") || !config["scene"]["sources"].is_array())
			return scene;
		for (const auto& item : config["scene"]["sources"])
			scene.append(item.get<StreamSource>());
		return scene;
	}

	void saveSceneJson(const QList<StreamSource>& scene)
	{
		nlohmann::json arr = nlohmann::json::array();
		for (const auto& source : scene)
		{
			nlohmann::json item = source;
			arr.push_back(item);
		}
		thatboy::storage::config["scene"]["sources"] = arr;
	}
}

LivePlatform::LivePlatform(QWidget* parent)
	: DragableMainWindow(parent)
{
	ui.setupUi(this);

	thatboy::storage::config["widget_info"]["platform"].get<thatboy::WidgetConfigInfo>().config(*this);

	m_scene = loadScene(thatboy::storage::config);
	m_sceneSeeded = thatboy::storage::config.contains("scene");

	refreshDevices();
	syncSourceList();
	ui.previewLayout->addWidget(&previewWidget);
	appendStatus(tr("从左侧“＋”添加采集源"));

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
			// bitrate and fps while streaming.
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

			syncPreview(); // bring the preview back
		});

	connect(ui.startPushButton, &QPushButton::clicked, this, &LivePlatform::startStreaming);
	connect(ui.stopPushButton, &QPushButton::clicked, this, &LivePlatform::stopStreaming);

	// Source dock toolbar.
	auto* addMenu = new QMenu(this);
	addMenu->addAction(QIcon(), tr("摄像头"), this, [this]
		{
			StreamSource source;
			source.kind = StreamSource::Kind::Camera;
			source.name = cameraDeviceName();
			source.title = ui.cameraComboBox->currentText();
			if (source.name.isEmpty())
			{
				appendStatus(tr("没有可用的摄像头"));
				return;
			}
			addSource(source);
		});
	addMenu->addAction(tr("整个屏幕"), this, [this]
		{
			StreamSource source;
			source.kind = StreamSource::Kind::Desktop;
			addSource(source);
		});
	addMenu->addAction(tr("窗口…"), this, [this]
		{
			const QString title = pickWindowTitle();
			if (title.isEmpty())
				return;
			StreamSource source;
			source.kind = StreamSource::Kind::Window;
			source.name = title;
			addSource(source);
		});
	addMenu->addAction(tr("本地文件…"), this, [this]
		{
			const QString file = QFileDialog::getOpenFileName(this, tr("选择推流文件"), QString(),
				tr("媒体文件 (*)"));
			if (file.isEmpty())
				return;
			StreamSource source;
			source.kind = StreamSource::Kind::LocalFile;
			source.name = file;
			addSource(source);
		});
	ui.addSourceToolButton->setMenu(addMenu);
	ui.addSourceToolButton->setPopupMode(QToolButton::InstantPopup);
	connect(ui.removeSourceToolButton, &QToolButton::clicked, this, &LivePlatform::removeSource);
	connect(ui.moveUpToolButton, &QToolButton::clicked, this, [this]
		{
			moveSource(-1);
		});
	connect(ui.moveDownToolButton, &QToolButton::clicked, this, [this]
		{
			moveSource(1);
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
			previewWidget.stopAll();
		else if (isVisible())
			syncPreview();
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
	syncPreview();
}

void LivePlatform::hideToTray()
{
	hide();
	previewWidget.stopAll();
}

QString LivePlatform::cameraDeviceName() const
{
	const int index = ui.cameraComboBox->currentIndex();
	return index >= 0 ? ui.cameraComboBox->itemData(index).toString() : QString();
}

QString LivePlatform::pickWindowTitle()
{
	QDialog picker(this);
	picker.setWindowTitle(tr("选择要捕获的窗口"));
	picker.setMinimumSize(420, 360);
	auto* layout = new QVBoxLayout(&picker);
	auto* list = new QListWidget(&picker);
	list->setFocusPolicy(Qt::ClickFocus);
	for (const WindowInfo& window : visibleTopLevelWindows())
		list->addItem(window.title);
	layout->addWidget(list);
	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &picker);
	layout->addWidget(buttons);
	QString chosen;
	connect(buttons, &QDialogButtonBox::accepted, &picker, [&]
		{
			chosen = list->currentItem() ? list->currentItem()->text() : QString();
			picker.accept();
		});
	connect(buttons, &QDialogButtonBox::rejected, &picker, &QDialog::reject);
	picker.exec();
	return chosen;
}

void LivePlatform::persistScene()
{
	saveSceneJson(m_scene);
}

void LivePlatform::addSource(const StreamSource& source)
{
	m_scene.append(source);
	persistScene();
	syncSourceList();
	syncPreview();
}

void LivePlatform::removeSource()
{
	const int row = ui.sourceListWidget->currentRow();
	if (row < 0 || row >= m_scene.size())
		return;
	m_scene.removeAt(row);
	persistScene();
	syncSourceList();
	syncPreview();
}

void LivePlatform::moveSource(int delta)
{
	const int row = ui.sourceListWidget->currentRow();
	const int target = row + delta;
	if (row < 0 || target < 0 || target >= m_scene.size())
		return;
	m_scene.move(row, target);
	persistScene();
	syncSourceList();
	ui.sourceListWidget->setCurrentRow(target);
	syncPreview();
}

void LivePlatform::syncSourceList()
{
	const QSignalBlocker blocker(ui.sourceListWidget);
	const QString selected = ui.sourceListWidget->currentItem()
		? ui.sourceListWidget->currentItem()->text() : QString();
	ui.sourceListWidget->clear();
	for (const StreamSource& source : m_scene)
		ui.sourceListWidget->addItem(source.displayTitle());
	if (!selected.isEmpty())
	{
		const QList<QListWidgetItem*> matches = ui.sourceListWidget->findItems(selected, Qt::MatchExactly);
		if (!matches.isEmpty())
			ui.sourceListWidget->setCurrentItem(matches.front());
	}
}

void LivePlatform::syncPreview()
{
	if (ffmpegProcess.state() == QProcess::Running)
		return;
	previewWidget.setSources(m_scene);
}

void LivePlatform::startStreaming()
{
	if (ffmpegProcess.state() != QProcess::NotRunning)
		return;

	StreamSettings settings;
	settings.server = streamSetting("server", "rtmp://rtmp.ohtoai.top");
	settings.microphoneName = streamSetting("microphone", "virtual-audio-capturer");
	settings.size = streamSetting("size", "1280x720");
	settings.encoder = streamSetting("encoder", "Auto");
	settings.bitrate = streamSetting("bitrate", "Auto");
	settings.fps = streamSetting("fps", "Auto");
	settings.watermark = thatboy::storage::config["stream"].value("watermark", true);
	settings.watermarkFile = logoImage;

	QString error;
	const QStringList arguments = buildSceneArgs(m_scene, settings, &error);
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

	previewWidget.stopAll(); // dshow devices cannot be opened twice
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
	ui.sourceListWidget->setEnabled(!streaming);
	ui.addSourceToolButton->setEnabled(!streaming);
	ui.removeSourceToolButton->setEnabled(!streaming);
	ui.moveUpToolButton->setEnabled(!streaming);
	ui.moveDownToolButton->setEnabled(!streaming);
	ui.cameraComboBox->setEnabled(!streaming);
	ui.startPushButton->setVisible(!streaming);
	ui.stopPushButton->setVisible(streaming);
}

void LivePlatform::appendStatus(const QString& text)
{
	statusBar()->showMessage(text);
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
		const QSignalBlocker blocker(ui.cameraComboBox);
		ui.cameraComboBox->clear();
		for (const auto& camera : cameras)
			ui.cameraComboBox->addItem(QString::fromStdString(camera->device_description),
				QString::fromStdString(camera->device_name));
	}

	// Seed a default camera scene exactly once, for fresh installs.
	if (!m_sceneSeeded)
	{
		m_sceneSeeded = true;
		if (m_scene.isEmpty() && ui.cameraComboBox->count() > 0)
		{
			StreamSource source;
			source.kind = StreamSource::Kind::Camera;
			source.name = ui.cameraComboBox->itemData(0).toString();
			source.title = ui.cameraComboBox->itemText(0);
			m_scene.append(source);
			persistScene();
			syncSourceList();
			syncPreview();
		}
	}
}
