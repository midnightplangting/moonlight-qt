#include "Logger.h"
#include "nvcomputer.h"
#include "computermanager.h"
#include <QDebug>
#include <QStringList>
#include <QReadWriteLock>
#include <QThread>

LogLevel Logger::s_level = LogLevel::Info;
bool Logger::s_enabled = true;

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

    Logger::log(LogLevel::Info, QStringLiteral("[ComputerDump] %1").arg(items.join(' ')));
}
