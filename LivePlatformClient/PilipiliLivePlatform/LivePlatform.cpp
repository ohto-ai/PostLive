#include "LivePlatform.h"
#include <QCameraInfo>

LivePlatform::LivePlatform(QWidget *parent)
    : DragableMainWindow(parent)
{
    ui.setupUi(this);
	setAttribute(Qt::WA_TranslucentBackground);
	connect(ui.closeToolButton, &QToolButton::clicked, this, &LivePlatform::close);
	connect(ui.minimizeToolButton, &QToolButton::clicked, this, &LivePlatform::showMinimized);


	connect(&ffmpegProcess, &QProcess::started, [&]
		{
			ui.startPushButton->setEnabled(false);
			ui.stopPushButton->setEnabled(true);
		}
	);
	connect(&ffmpegProcess, static_cast<void(QProcess::*)(int)>(&QProcess::finished), [&](int code)
		{
			ui.startPushButton->setEnabled(true);
			ui.stopPushButton->setEnabled(false);
		}
	);

	connect(ui.startPushButton, &QPushButton::clicked, [&]
		{
			auto command = QString::asprintf(
				R"(ffmpeg.exe -f dshow -i video="%s" -i %s -filter_complex "overlay=5:5" -f dshow -i audio="%s" -f %s -s %dx%d rtmp://%s%s/%s)"
				, ui.cameraComboBox->currentText().toStdString().c_str()
				, "thatboylogo.png"
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

	auto deviceList = QCameraInfo::availableCameras();

	for (auto& device : deviceList)
		ui.cameraComboBox->addItem(device.description());
}
