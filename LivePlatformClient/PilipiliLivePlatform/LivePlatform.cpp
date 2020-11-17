#if defined(_MSC_VER) && (_MSC_VER >= 1600)    
# pragma execution_character_set("utf-8")    
#endif

#include "LivePlatform.h"

#include <future>
#include <QMessageBox>
#include <QMenu>
#include <QAudioDeviceInfo>
#include <QDesktopServices>

Q_DECLARE_METATYPE(QCameraInfo)

LivePlatform::LivePlatform(QWidget* parent)
	: DragableMainWindow(parent)
{
	ui.setupUi(this);

	thatboy::storage::config["widget_info"]["platform"].get<thatboy::WidgetConfigInfo>().config(*this);

	auto mSysTrayIcon = new QSystemTrayIcon(this);
	mSysTrayIcon->setIcon(QIcon(":/LiveLoginDialog/res/live.ico"));
	mSysTrayIcon->setToolTip(QObject::trUtf8("LivePlatform"));

	auto m_menu = new QMenu(this);
	auto actionShowMainWindow = new QAction(QIcon(":/LiveLoginDialog/res/live.ico"), "显示主窗口", m_menu);
	auto actionViewWebsite = new QAction(QIcon(":/LiveLoginDialog/res/boy.ico"), "浏览官网", m_menu);
	auto actionViewGithub = new QAction(QIcon(":/LiveLoginDialog/res/github.png"), "项目地址", m_menu);
	auto actionExit = new QAction("退出客户端", m_menu);

	m_menu->addAction(actionShowMainWindow);
	m_menu->addAction(actionViewWebsite);
	m_menu->addAction(actionViewGithub);
	m_menu->addAction(actionExit);

	connect(actionShowMainWindow, &QAction::triggered, [=]
	{
			if (isHidden())
			{
				if (ffmpegProcess.state() != QProcess::Running)
				{
					viewCamera->start();
				}
				show();
			}
	});
	connect(actionViewWebsite, &QAction::triggered, std::bind(&QDesktopServices::openUrl, QUrl(thatboy::storage::WebsiteUrl)));
	connect(actionViewGithub, &QAction::triggered, std::bind(&QDesktopServices::openUrl, QUrl(thatboy::storage::GithubUrl)));
	connect(actionExit, &QAction::triggered, [=] {close(); });

	mSysTrayIcon->setContextMenu(m_menu);
	connect(mSysTrayIcon, &QSystemTrayIcon::activated, [&, actionShowMainWindow](QSystemTrayIcon::ActivationReason reason)
		{
			switch (reason) {
			case QSystemTrayIcon::DoubleClick:
				if (isHidden())
				{
					if (ffmpegProcess.state() != QProcess::Running)
					{
						viewCamera->start();
					}
					show();
				}
				else
				{
					hide();
					if (ffmpegProcess.state() != QProcess::Running)
					{
						viewCamera->stop();
					}
				}
				break;
			default:
				break;
			}
		});
	mSysTrayIcon->show();

	ui.streamServerLineEdit->setValidator(new QRegExpValidator(
		QRegExp{ R"(^((?=^.{3,255}$)[a-zA-Z0-9][-a-zA-Z0-9]{0,62}(\.[a-zA-Z0-9][-a-zA-Z0-9]{0,62})+(:\d+)?)|(((25[0-5])|(2[0-4]d)|(1dd)|([1-9]d)|d)(.((25[0-5])|(2[0-4]d)|(1dd)|([1-9]d)|d)){3})|(localhost(:\d+)?)$)" }, this));
	
	connect(this, &BaseMainWindow::closed, [&]
		{
			thatboy::storage::config["widget_info"]["platform"] = thatboy::WidgetConfigInfo(*this);
		});
	setAttribute(Qt::WA_TranslucentBackground);
	connect(ui.closeToolButton, &QToolButton::clicked, [&]
		{
			hide();
			if (ffmpegProcess.state() != QProcess::Running)
			{
				viewCamera->stop();
			}
		});
	connect(ui.minimizeToolButton, &QToolButton::clicked, this, &LivePlatform::showMinimized);

	connect(&ffmpegProcess, &QProcess::started, [&]
		{
			ui.cameraComboBox->setEnabled(false);
			ui.microphoneComboBox->setEnabled(false);
			ui.streamServerLineEdit->setEnabled(false);
			ui.sizeComboBox->setEnabled(false);
			ui.startPushButton->setEnabled(false);
			ui.stopPushButton->setEnabled(true);
		}
	);
	connect(&ffmpegProcess, static_cast<void(QProcess::*)(int)>(&QProcess::finished), [&](int code)
		{
			ui.cameraComboBox->setEnabled(true);
			ui.microphoneComboBox->setEnabled(true);
			ui.streamServerLineEdit->setEnabled(true);
			ui.sizeComboBox->setEnabled(true);
			ui.startPushButton->setEnabled(true);
			ui.stopPushButton->setEnabled(false);
			viewCamera->start();
		}
	);

	connect(ui.startPushButton, &QPushButton::clicked, [&]
		{
			if (!ui.streamServerLineEdit->hasAcceptableInput())
			{
				QMessageBox::warning(this, "错误", "无效的服务器地址！");
				return;
			}
			viewCamera->stop();
			auto command = QString::asprintf(
				R"(%s -f dshow -i video="%s" -i %s -filter_complex "overlay=5:5" -f dshow -i audio="%s" -f %s -s %s rtmp://%s/live/%s)"
				, ffmpegPath.toStdString().c_str()
				, ui.cameraComboBox->currentText().toStdString().c_str()
				, logoImage.toStdString().c_str()
				, ui.microphoneComboBox->currentText().toStdString().c_str()
				, videoFormt.toStdString().c_str()
				, ui.sizeComboBox->currentText().toStdString().c_str()
				, ui.streamServerLineEdit->text().toStdString().c_str()
				, thatboy::storage::currentUser["account"].get<std::string>().c_str());
			ffmpegProcess.start(command);
		});

	connect(ui.stopPushButton, &QPushButton::clicked, [&]
		{
			QTimer::singleShot(1000, std::bind(&QProcess::close, &ffmpegProcess));
			ffmpegProcess.write("q");	
		});

	auto cameraList = QCameraInfo::availableCameras();

	for (auto& cameraInfo : cameraList)
		ui.cameraComboBox->addItem(cameraInfo.description(), QVariant::fromValue(cameraInfo));
	ui.cameraComboBox->setCurrentText(QCameraInfo::defaultCamera().description());

	auto microphoneList = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
	QSet<QString> microphoneNameList;
	for (auto& microphoneInfo : microphoneList)
		microphoneNameList.insert(microphoneInfo.deviceName());

	microphoneNameList.insert("virtual-audio-capturer");
	
	for (auto& microphoneName : microphoneNameList)
		ui.microphoneComboBox->addItem(microphoneName);
	ui.microphoneComboBox->setCurrentText(QAudioDeviceInfo::defaultInputDevice().deviceName());

	ui.sizeComboBox->addItems({
		"640×480", "800×600", "1024×768", "1280x720", "1280×800", "1400×1050", "1440×900", "1600×1200", "1920x1080", "1920×1200"
		});
	ui.sizeComboBox->setCurrentText("1280x720");

	
	ui.cameraViewLayout->addWidget(&cameraViewfinder);

	connect(ui.cameraComboBox, static_cast<void(QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged)
		, [&](const QString& text)
		{
			setViewCamera(qvariant_cast<QCameraInfo>(ui.cameraComboBox->currentData()));
			viewCamera->start();
		});

	setViewCamera(QCameraInfo::defaultCamera());
	viewCamera->start();
}

void LivePlatform::setViewCamera(const QCameraInfo& cameraInfo)
{
	delete viewCamera;
	viewCamera = new QCamera(cameraInfo, this);
	viewCamera->setViewfinder(&cameraViewfinder);
}