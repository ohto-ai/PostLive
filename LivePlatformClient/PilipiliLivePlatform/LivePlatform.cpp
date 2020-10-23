#include "LivePlatform.h"

Q_DECLARE_METATYPE(QCameraInfo)
LivePlatform::LivePlatform(QWidget* parent)
	: DragableMainWindow(parent)
{
	ui.setupUi(this);

	thatboy::storage::config["widget_info"]["platform"].get<thatboy::WidgetConfigInfo>().config(*this);

	auto mSysTrayIcon = new QSystemTrayIcon(this);
	mSysTrayIcon->setIcon(QIcon(":/LiveLoginDialog/res/live.ico"));
	mSysTrayIcon->setToolTip(QObject::trUtf8("LivePlatform"));
	connect(mSysTrayIcon, &QSystemTrayIcon::activated, [&](QSystemTrayIcon::ActivationReason reason)
		{
			switch (reason) {
			case QSystemTrayIcon::Trigger:
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
			case QSystemTrayIcon::DoubleClick:
				this->close();
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
			ui.startPushButton->setEnabled(false);
			ui.stopPushButton->setEnabled(true);
		}
	);
	connect(&ffmpegProcess, static_cast<void(QProcess::*)(int)>(&QProcess::finished), [&](int code)
		{
			ui.cameraComboBox->setEnabled(true);
			ui.startPushButton->setEnabled(true);
			ui.stopPushButton->setEnabled(false);
			viewCamera->start();
		}
	);

	connect(ui.startPushButton, &QPushButton::clicked, [&]
		{
			viewCamera->stop();
			auto command = QString::asprintf(
				R"(bin\ffmpeg.exe -f dshow -i video="%s" -i %s -filter_complex "overlay=5:5" -f dshow -i audio="%s" -f %s -s %dx%d rtmp://%s%s/%s)"
				, ui.cameraComboBox->currentText().toStdString().c_str()
				, "assets\\thatboylogo.png"
				, "virtual-audio-capturer"
				, "flv"
				, 1920, 1080
				, "localhost:1935"
				, "/hls"
				, thatboy::storage::currentUser["account"].get<std::string>().c_str());
			ffmpegProcess.start(command);
		});

	connect(ui.stopPushButton, &QPushButton::clicked, [&]
		{
			ffmpegProcess.write("q");
		});

	auto cameraList = QCameraInfo::availableCameras();

	for (auto& cameraInfo : cameraList)
		ui.cameraComboBox->addItem(cameraInfo.description(), QVariant::fromValue(cameraInfo));
	ui.cameraComboBox->setCurrentText(QCameraInfo::defaultCamera().description());

	ui.cameraViewLayout->addWidget(&cameraViewfinder);

	connect(ui.cameraComboBox, static_cast<void(QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged), [&](const QString& text)
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