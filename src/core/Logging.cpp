#include "Logging.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>

Q_LOGGING_CATEGORY(lcApp, "postlive.app")
Q_LOGGING_CATEGORY(lcCore, "postlive.core")
Q_LOGGING_CATEGORY(lcNetwork, "postlive.network")
Q_LOGGING_CATEGORY(lcStream, "postlive.stream")
Q_LOGGING_CATEGORY(lcDevice, "postlive.device")

namespace thatboy
{
	namespace log
	{
		namespace
		{
			constexpr qint64 MaxLogBytes{ 2 * 1024 * 1024 };
			constexpr int KeepBackups{ 1 };

			QMutex g_mutex;
			QFile g_file;
			qint64 g_written{ 0 };
			bool g_installed{ false };

			QString g_logDirectory; // set by initLogging

			QString logFilePath()
			{
				return g_logDirectory + QStringLiteral("/PostLive.log");
			}

			const char* typeName(QtMsgType type)
			{
				switch (type)
				{
				case QtDebugMsg: return "debug";
				case QtInfoMsg: return "info";
				case QtWarningMsg: return "warning";
				case QtCriticalMsg: return "critical";
				case QtFatalMsg: return "fatal";
				}
				return "unknown";
			}

			QString formatMessage(QtMsgType type, const QMessageLogContext& context, const QString& message)
			{
				QString category = QString::fromLatin1(context.category);
				if (category.isEmpty() || category == QLatin1String("default"))
					category = QStringLiteral("app");

				return QStringLiteral("%1 [%2] %3: %4")
					.arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")))
					.arg(QString::fromLatin1(typeName(type)))
					.arg(category)
					.arg(message);
			}

			bool openLocked()
			{
				const QString path = logFilePath();
				QDir().mkpath(QFileInfo(path).absolutePath());

				// Rotate an oversized log from a previous run out of the way.
				QFileInfo info(path);
				if (info.size() > MaxLogBytes)
				{
					QFile::remove(path + QStringLiteral(".1"));
					QFile::rename(path, path + QStringLiteral(".1"));
				}

				g_file.setFileName(path);
				const bool ok = g_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
				g_written = ok ? g_file.size() : 0;
				return ok;
			}

			void rotateLocked()
			{
				g_file.close();
				const QString path = logFilePath();
				QFile::remove(path + QStringLiteral(".1"));
				QFile::rename(path, path + QStringLiteral(".1"));
				openLocked();
			}

			void writeMessage(QtMsgType type, const QMessageLogContext& context, const QString& message)
			{
				QMutexLocker locker(&g_mutex);
				const QString line = formatMessage(type, context, message);

				{
					const QByteArray encoded = line.toUtf8() + '\n';
					if (g_file.isOpen())
					{
						g_file.write(encoded);
						g_written += encoded.size();
						if (g_written > MaxLogBytes)
							rotateLocked();
						else
							g_file.flush();
					}
				}

				fprintf(stderr, "%s\n", line.toLocal8Bit().constData());
				fflush(stderr);

				if (type == QtFatalMsg)
				{
					g_file.flush();
					abort();
				}
			}

			void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message)
			{
				if (!g_installed)
				{
					// Handler installed but logging not initialized yet: stderr only.
					fprintf(stderr, "%s\n", message.toLocal8Bit().constData());
					return;
				}
				writeMessage(type, context, message);
			}
		}

		void initLogging(const QString& directory)
		{
			QMutexLocker locker(&g_mutex);
			g_logDirectory = directory;
			QDir().mkpath(directory);
			openLocked();
			g_installed = true;
			qInstallMessageHandler(messageHandler);
		}
	}
}
