#if defined(_MSC_VER) && (_MSC_VER >= 1600)
# pragma execution_character_set("utf-8")
#endif

#include "LivePlatform.h"
#include "FFmpegInputDevice.h"
#include "LiveLoginSettingDialog.h"
#include "capture/DesktopDuplicationCapture.h"
#include "../core/Logging.h"
#include "../platform/WindowList.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QShortcut>
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
	ui.audioMicCheckBox->setChecked(
		thatboy::storage::config["stream"].value("audio_enabled", true));

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
			if (sceneEncoder.isRunning())
				sceneEncoder.stop();
			if (m_streaming)
				stopStreaming();
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
			thatboy::storage::config["stream"]["audio_enabled"] = ui.audioMicCheckBox->isChecked();
		});
	setAttribute(Qt::WA_TranslucentBackground);
	connect(ui.closeToolButton, &QToolButton::clicked, this, &LivePlatform::hideToTray);
	connect(ui.minimizeToolButton, &QToolButton::clicked, this, &LivePlatform::showMinimized);
	connect(ui.settingsPushButton, &QPushButton::clicked, this, &LivePlatform::openSettings);

	// Encoder lifecycle → control dock.
	connect(&sceneEncoder, &SceneEncoder::streamStarted, this, [this]
		{
			m_streaming = true;
			updateStreamControls(true);
			appendStatus(tr("推流中"));
		});
	connect(&sceneEncoder, &SceneEncoder::streamStats, this, [this](int seconds)
		{
			ui.progressLabel->setText(tr("直播 %1:%2")
				.arg(seconds / 60, 2, 10, QLatin1Char('0'))
				.arg(seconds % 60, 2, 10, QLatin1Char('0')));
		});
	connect(&sceneEncoder, &SceneEncoder::streamStopped, this, [this]
		{
			if (m_streaming)
			{
				m_streaming = false;
				updateStreamControls(false);
				ui.progressLabel->clear();
				appendStatus(tr("推流已停止"));
				syncPreview(); // bring the preview back
			}
		});
	connect(&sceneEncoder, &SceneEncoder::errorOccurred, this, [this](const QString& message)
		{
			m_streaming = false;
			updateStreamControls(false);
			appendStatus(message);
		});
	// Audio meter + mute.
	connect(&sceneEncoder, &SceneEncoder::audioLevel, ui.audioLevelBar,
		&AudioLevelBar::setLevel);
	connect(ui.mutePushButton, &QPushButton::toggled, &sceneEncoder,
		&SceneEncoder::setMuted);
	connect(ui.mutePushButton, &QPushButton::toggled, this, [this](bool muted)
		{
			ui.mutePushButton->setText(muted ? tr("取消静音") : tr("静音"));
		});

	connect(ui.startPushButton, &QPushButton::clicked, this, &LivePlatform::startStreaming);
	connect(ui.stopPushButton, &QPushButton::clicked, this, &LivePlatform::stopStreaming);

	// OBS-style: the composited preview canvas is the stream input, so the
	// preview keeps running while live and edits apply immediately. The
	// in-process encoder consumes the canvas frames directly.
	connect(&previewWidget, &ScenePreview::canvasReady,
		&sceneEncoder, &SceneEncoder::onCanvasReady);
	connect(&previewWidget, &ScenePreview::sceneChanged, this, [this](QList<StreamSource> sources)
		{
			m_scene = sources;
			persistScene();
			syncSourceList();
		});
	connect(&previewWidget, &ScenePreview::sourceSelected, this, [this](int index)
		{
			const QSignalBlocker blocker(ui.sourceListWidget);
			ui.sourceListWidget->setCurrentRow(index);
		});
	connect(ui.sourceListWidget, &QListWidget::currentRowChanged, this, [this](int row)
		{
			previewWidget.setSelectedIndex(row);
		});

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
	const auto monitors = DesktopDuplicationCapture::availableMonitors();
	for (int i = 0; i < monitors.size(); ++i)
	{
		const DesktopDuplicationCapture::MonitorInfo monitor = monitors[i];
		const QString label = monitor.primary
			? tr("主屏幕 (%1x%2)").arg(monitor.geometry.width()).arg(monitor.geometry.height())
			: tr("屏幕 %1 (%2x%3)").arg(i + 1)
				.arg(monitor.geometry.width()).arg(monitor.geometry.height());
		addMenu->addAction(label, this, [this, monitor]
			{
				StreamSource source;
				source.kind = StreamSource::Kind::Desktop;
				source.name = monitor.deviceName;
				source.title = QString("%1x%2").arg(monitor.geometry.width())
					.arg(monitor.geometry.height());
				addSource(source);
			});
	}
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
	connect(ui.fitSourceToolButton, &QToolButton::clicked, this, [this]
		{
			const int row = ui.sourceListWidget->currentRow();
			if (row < 0 || row >= m_scene.size())
				return;
			m_scene[row].rect = QRect(QPoint(0, 0), previewWidget.canvasSize());
			persistScene();
			syncPreview();
		});
	// OBS-style source list context menu and Delete shortcut.
	connect(ui.sourceListWidget, &QListWidget::customContextMenuRequested, this,
		[this](const QPoint& pos)
		{
			if (ui.sourceListWidget->itemAt(pos) == nullptr)
				return;
			QMenu menu(this);
			menu.addAction(tr("适配画布"), this, [this]
				{
					emit ui.fitSourceToolButton->clicked();
				});
			menu.addAction(tr("移除"), this, &LivePlatform::removeSource);
			menu.addSeparator();
			menu.addAction(tr("上移"), this, [this]
				{
					moveSource(-1);
				});
			menu.addAction(tr("下移"), this, [this]
				{
					moveSource(1);
				});
			menu.exec(ui.sourceListWidget->mapToGlobal(pos));
		});
	auto* deleteShortcut = new QShortcut(QKeySequence::Delete, this);
	connect(deleteShortcut, &QShortcut::activated, this, [this]
		{
			if (ui.sourceListWidget->hasFocus() || !ui.sourceListWidget->selectedItems().isEmpty())
				removeSource();
		});

	connect(&deviceRefreshTimer, &QTimer::timeout, this, &LivePlatform::refreshDevices);
	deviceRefreshTimer.start(3000);

	// Headless e2e hook: POSTLIVE_SMOKE_STREAM=1 auto-starts streaming with
	// local HLS output (POSTLIVE_SMOKE_CAPTURE picks the source).
	if (qEnvironmentVariableIsSet("POSTLIVE_SMOKE_STREAM"))
	{
		m_smokeStream = true;
		thatboy::storage::config["stream"]["local_hls"] = true;
		// POSTLIVE_SMOKE_NOAUDIO=1: video-only stream (diagnoses muxer stalls).
		if (qEnvironmentVariableIsSet("POSTLIVE_SMOKE_NOAUDIO"))
			ui.audioMicCheckBox->setChecked(false);
		QTimer::singleShot(800, this, [this]
			{
				startStreaming();
			});
	}

	// Headless capture test hook (CI / diagnostics): POSTLIVE_SMOKE_CAPTURE
	// = "desktop" or "window:<exact title>" adds that source on startup.
	const QString smokeCapture = qEnvironmentVariable("POSTLIVE_SMOKE_CAPTURE");
	if (!smokeCapture.isEmpty())
	{
		StreamSource source;
		if (smokeCapture == QLatin1String("desktop"))
		{
			const auto monitors = DesktopDuplicationCapture::availableMonitors();
			if (!monitors.isEmpty())
			{
				source.kind = StreamSource::Kind::Desktop;
				source.name = monitors.front().deviceName;
				source.title = QString("%1x%2").arg(monitors.front().geometry.width())
					.arg(monitors.front().geometry.height());
			}
		}
		else if (smokeCapture.startsWith(QLatin1String("window:")))
		{
			source.kind = StreamSource::Kind::Window;
			source.name = smokeCapture.mid(QStringLiteral("window:").size());
		}
		if (!source.name.isEmpty())
			addSource(source);
	}
}

LivePlatform::~LivePlatform()
{
	if (m_streaming)
	{
		sceneEncoder.stop();
		m_streaming = false;
	}
}

void LivePlatform::openSettings()
{
	auto* dialog = new LiveLoginSettingDialog(this);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	connect(dialog, &QDialog::accepted, this, [this]
		{
			const QString size = streamSetting("size", "1280x720");
			previewWidget.setCanvasSize(QSize(size.section('x', 0, 0).toInt(),
				size.section('x', 1, 1).toInt()));
		});
	dialog->show();
	dialog->raise();
	dialog->activateWindow();
}

void LivePlatform::changeEvent(QEvent* event)
{
	if (event->type() == QEvent::WindowStateChange)
	{
		// While streaming the preview feeds the stream: keep it alive.
		if (!m_streaming)
		{
			if (isMinimized())
				previewWidget.stopAll();
			else if (isVisible())
				syncPreview();
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
	syncPreview();
}

void LivePlatform::hideToTray()
{
	hide();
	if (!m_streaming)
		previewWidget.stopAll();
}

QString LivePlatform::cameraDeviceName() const
{
	const int index = ui.cameraComboBox->currentIndex();
	return index >= 0 ? ui.cameraComboBox->itemData(index).toString() : QString();
}

// Configured microphone first; on machines without it (or the legacy
// virtual-audio-capturer default) prefer a real capture device over the
// wave-mapper pseudo device, which some setups deliver no packets on.
QString LivePlatform::effectiveMicrophoneName() const
{
	QString configured = streamSetting("microphone", "virtual-audio-capturer");
	FFmpegInputDevice::updateInputDeviceList();
	QStringList realDevices;
	QStringList allDevices;
	for (const FFmpegInputDevice* microphone : FFmpegInputDevice::filterAudioDevices())
	{
		const QString name = QString::fromStdString(microphone->device_name);
		allDevices << name;
		if (!name.startsWith(QStringLiteral("@device_")))
			realDevices << name;
	}
	if (allDevices.contains(configured))
		return configured;
	if (!realDevices.isEmpty())
		return realDevices.first();
	return allDevices.isEmpty() ? QString() : allDevices.first();
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
	ui.sourceListWidget->setCurrentRow(m_scene.size() - 1);
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
	previewWidget.setSources(m_scene);
}

void LivePlatform::startStreaming()
{
	if (m_streaming)
		return;
	if (m_scene.isEmpty())
	{
		appendStatus(tr("请先添加采集源"));
		return;
	}

	// Both outputs are produced by the in-process encoder. Local HLS is the
	// default; the RTMP output pushes FLV over avio (no child process).
	const bool localHls = m_smokeStream
		|| thatboy::storage::config["stream"].value("local_hls", true);
	const QString fpsSetting = streamSetting("fps", "Auto");
	const QString bitrateSetting = streamSetting("bitrate", "Auto");
	const int fpsValue = fpsSetting == QStringLiteral("Auto")
		? 30 : qBound(1, fpsSetting.toInt(), 120);
	const int bitrateKbps = bitrateSetting == QStringLiteral("Auto")
		? 2500 : qBound(100, bitrateSetting.toInt(), 20000);
	const QString encoder = streamSetting("encoder", "Auto");
	const QString microphone = effectiveMicrophoneName();
	const bool withAudio = ui.audioMicCheckBox->isChecked() && !microphone.isEmpty();

	if (localHls)
	{
		const QString hlsDir = thatboy::storage::resolveDataPath(QStringLiteral("hls"));
		QDir().mkpath(hlsDir);
		sceneEncoder.startStream(SceneEncoder::OutputMode::LocalHls, hlsDir,
			previewWidget.canvasSize(), fpsValue, encoder, bitrateKbps,
			withAudio, microphone);
		appendStatus(tr("本地观看：http://localhost:18080/player.html"));
	}
	else
	{
		QString url = streamSetting("server", "rtmp://rtmp.ohtoai.top").trimmed();
		while (url.endsWith(QLatin1Char('/')))
			url.chop(1);
		sceneEncoder.startStream(SceneEncoder::OutputMode::Rtmp, url,
			previewWidget.canvasSize(), fpsValue, encoder, bitrateKbps,
			withAudio, microphone);
		appendStatus(tr("正在连接推流服务器..."));
	}
}

void LivePlatform::stopStreaming()
{
	if (!m_streaming)
		return;
	// Blocking stop: the encoder finishes its current frame, flushes both
	// encoders and writes the trailer (no child process involved).
	sceneEncoder.stop();
	m_streaming = false;
	updateStreamControls(false);
	ui.progressLabel->clear();
	appendStatus(tr("推流已停止"));
}

void LivePlatform::updateStreamControls(bool streaming)
{
	// Source editing stays enabled while live: the canvas is the stream
	// input, so changes apply immediately (OBS-style).
	ui.startPushButton->setVisible(!streaming);
	ui.stopPushButton->setVisible(streaming);
}

void LivePlatform::appendStatus(const QString& text)
{
	statusBar()->showMessage(text);
}

void LivePlatform::refreshDevices()
{
	if (m_streaming)
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

	// Seed a default scene exactly once, for fresh installs.
	if (!m_sceneSeeded)
	{
		m_sceneSeeded = true;
		if (m_scene.isEmpty() && !thatboy::storage::config.contains("scene"))
		{
			if (ui.cameraComboBox->count() > 0)
			{
				StreamSource source;
				source.kind = StreamSource::Kind::Camera;
				source.name = ui.cameraComboBox->itemData(0).toString();
				source.title = ui.cameraComboBox->itemText(0);
				m_scene.append(source);
			}
			else
			{
				const auto monitors = DesktopDuplicationCapture::availableMonitors();
				if (!monitors.isEmpty())
				{
					StreamSource source;
					source.kind = StreamSource::Kind::Desktop;
					source.name = monitors.front().deviceName;
					source.title = QString("%1x%2").arg(monitors.front().geometry.width())
						.arg(monitors.front().geometry.height());
					m_scene.append(source);
				}
			}
			if (!m_scene.isEmpty())
			{
				persistScene();
				syncSourceList();
				syncPreview();
			}
		}
	}
}
