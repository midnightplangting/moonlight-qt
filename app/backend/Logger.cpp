#include "Logger.h"
#include "nvcomputer.h"
#include "computermanager.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QMutexLocker>
#include <QReadWriteLock>
#include <QStringList>
#include <QStandardPaths>
#include <QTextStream>
#include <QThread>
#include <QVector>

LogLevel Logger::s_level = LogLevel::Error;
bool Logger::s_enabled = true;
QString Logger::s_logFilePath;
QMutex Logger::s_logMutex;

void Logger::setLevel(LogLevel level)
{
    s_level = level;
}

void Logger::enable(bool enable)
{
    s_enabled = enable;
}

void Logger::log(LogLevel level, const QString& msg)
{
    if (!s_enabled || static_cast<int>(level) > static_cast<int>(s_level))
        return;

    switch (level) {
    case LogLevel::Error:
        qCritical() << "[错误]" << msg;
        writeErrorLog(msg);
        break;
    case LogLevel::Warning:
        qWarning() << "[警告]" << msg;
        break;
    case LogLevel::Info:
        qInfo() << "[信息]" << msg;
        break;
    case LogLevel::Debug:
        qDebug() << "[调试]" << msg;
        break;
    }
}

void Logger::writeErrorLog(const QString& msg)
{
    QMutexLocker locker(&s_logMutex);

    if (s_logFilePath.isEmpty()) {
        auto prepareLogFile = [](const QString& directory) -> QString {
            if (directory.isEmpty()) {
                return QString();
            }

            QDir logDir(directory);
            if (!logDir.exists() && !logDir.mkpath(QStringLiteral("."))) {
                return QString();
            }

            const QFileInfoList existingFiles = logDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
            for (const QFileInfo& fileInfo : existingFiles) {
                logDir.remove(fileInfo.fileName());
            }

            const QString logFilePath = logDir.filePath(QStringLiteral("latest.log"));
            QFile file(logFilePath);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
                return QString();
            }
            file.close();

            return logFilePath;
        };

        struct LogCandidate {
            QString directory;
            QString description;
        };

        QVector<LogCandidate> candidates;

        const QString baseDir = QCoreApplication::applicationDirPath();
        if (!baseDir.isEmpty()) {
            candidates.append({ QDir(baseDir).filePath(QStringLiteral("logs")), QStringLiteral("安装目录") });
        }

#ifdef Q_OS_WIN
        const QString programData = qEnvironmentVariable("PROGRAMDATA");
        if (!programData.isEmpty()) {
            QStringList subPathParts;
            if (!QCoreApplication::organizationName().isEmpty()) {
                subPathParts << QCoreApplication::organizationName();
            }
            if (!QCoreApplication::applicationName().isEmpty()) {
                subPathParts << QCoreApplication::applicationName();
            }
            subPathParts << QStringLiteral("logs");

            const QString programDataLogs = QDir(programData).filePath(subPathParts.join(QStringLiteral("/")));
            candidates.append({ programDataLogs, QStringLiteral("公共数据目录") });
        }
#endif

        const QString appLocalDataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
        if (!appLocalDataDir.isEmpty()) {
            candidates.append({ QDir(appLocalDataDir).filePath(QStringLiteral("logs")), QStringLiteral("用户数据目录") });
        }

        const QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (!appDataDir.isEmpty() && appDataDir != appLocalDataDir) {
            candidates.append({ QDir(appDataDir).filePath(QStringLiteral("logs")), QStringLiteral("漫游数据目录") });
        }

        const QString homeDir = QDir::homePath();
        if (!homeDir.isEmpty()) {
            candidates.append({ QDir(homeDir).filePath(QStringLiteral("Moonlight/logs")), QStringLiteral("用户主目录") });
        }

        QString resolvedDescription;
        for (const LogCandidate& candidate : candidates) {
            const QString logFilePath = prepareLogFile(candidate.directory);
            if (!logFilePath.isEmpty()) {
                s_logFilePath = logFilePath;
                resolvedDescription = candidate.description;
                break;
            }
        }

        if (!s_logFilePath.isEmpty()) {
            qWarning() << "[日志] 错误日志写入" << resolvedDescription << ':' << s_logFilePath;
        }
        else {
            qWarning() << "[日志] 无法创建错误日志文件";
        }
    }

    if (s_logFilePath.isEmpty()) {
        return;
    }

    QFile file(s_logFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return;
    }

    QTextStream stream(&file);
    stream << QDateTime::currentDateTime().toString(Qt::ISODate) << " [错误] " << msg << '\n';
}

void Logger::logThread(LogLevel level, const QString& msg)
{
    QThread* thread = QThread::currentThread();
    QString threadName = thread->objectName();
    quintptr threadId = reinterpret_cast<quintptr>(QThread::currentThreadId());

    QString prefix;
    if (!threadName.isEmpty()) {
        prefix = QStringLiteral("[%1(%2)] ").arg(threadName).arg(threadId);
    }
    else {
        prefix = QStringLiteral("[Thread:%1] ").arg(threadId);
    }

    Logger::log(level, prefix + msg);
}

void Logger::logComputer(const NvComputer* computer, const DeviceInfo* info)
{
    if (!computer) {
        return;
    }

    QReadLocker lock(&computer->lock);

    QStringList items;
    items << QStringLiteral("name=%1").arg(computer->name)
          << QStringLiteral("uuid=%1").arg(computer->uuid)
          << QStringLiteral("state=%1").arg(static_cast<int>(computer->state))
          << QStringLiteral("pairState=%1").arg(static_cast<int>(computer->pairState))
          << QStringLiteral("local=%1:%2").arg(computer->localAddress.address()).arg(computer->localAddress.port())
          << QStringLiteral("remote=%1:%2").arg(computer->remoteAddress.address()).arg(computer->remoteAddress.port())
          << QStringLiteral("ipv6=%1:%2").arg(computer->ipv6Address.address()).arg(computer->ipv6Address.port())
          << QStringLiteral("manual=%1:%2").arg(computer->manualAddress.address()).arg(computer->manualAddress.port())
          << QStringLiteral("mac=%1").arg(QString(computer->macAddress.toHex(':')))
          << QStringLiteral("active=%1:%2").arg(computer->activeAddress.address()).arg(computer->activeAddress.port())
          << QStringLiteral("httpsPort=%1").arg(computer->activeHttpsPort)
          << QStringLiteral("currentGameId=%1").arg(computer->currentGameId)
          << QStringLiteral("gfeVersion=%1").arg(computer->gfeVersion)
          << QStringLiteral("appVersion=%1").arg(computer->appVersion)
          << QStringLiteral("maxLumaHEVC=%1").arg(computer->maxLumaPixelsHEVC)
          << QStringLiteral("codecSupport=%1").arg(computer->serverCodecModeSupport)
          << QStringLiteral("gpuModel=%1").arg(computer->gpuModel)
          << QStringLiteral("supported=%1").arg(computer->isSupportedServerVersion)
          << QStringLiteral("orderBitrate=%1").arg(computer->orderBitrate)
          << QStringLiteral("orderStartedAt=%1").arg(computer->orderStartedAt.toString(Qt::ISODate))
          << QStringLiteral("orderEndedAt=%1").arg(computer->orderEndedAt.toString(Qt::ISODate))
          << QStringLiteral("orderId=%1").arg(computer->orderId)
          << QStringLiteral("orderStatus=%1").arg(computer->orderStatus)
          << QStringLiteral("orderBillingType=%1").arg(computer->orderBillingType)
          << QStringLiteral("hasCustomName=%1").arg(computer->hasCustomName)
          << QStringLiteral("isNvidiaServerSoftware=%1").arg(computer->isNvidiaServerSoftware);

    if (info) {
        items << QStringLiteral("info.orderId=%1").arg(info->orderId)
              << QStringLiteral("info.bitrate=%1").arg(info->bitrate)
              << QStringLiteral("info.startedAt=%1").arg(info->startedAt.toString(Qt::ISODate))
              << QStringLiteral("info.endedAt=%1").arg(info->endedAt.toString(Qt::ISODate))
              << QStringLiteral("info.deviceGroupId=%1").arg(info->deviceGroupId)
              << QStringLiteral("info.status=%1").arg(info->status)
              << QStringLiteral("info.billingType=%1").arg(info->billingType);
    }

    // Polling dumps can be very frequent, so log them at debug level
    // to avoid spamming normal info logs.
    Logger::log(LogLevel::Debug, QStringLiteral("[ComputerDump] %1").arg(items.join(' ')));
}
