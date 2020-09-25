#pragma once

#include <QObject>

class GeneralSignal : public QObject
{
	Q_OBJECT
signals:
	void avatarDownloaded(QString user, bool success);
};
