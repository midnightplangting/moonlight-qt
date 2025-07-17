#include "Logger.h"
#include <QDebug>

LogLevel Logger::s_level = LogLevel::Error;
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
