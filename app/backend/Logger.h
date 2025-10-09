#ifndef LOGGER_H
#define LOGGER_H
#pragma once

#include <QMutex>
#include <QString>

class NvComputer;
struct DeviceInfo;

enum class LogLevel {
    Error = 0,
    Warning,
    Info,
    Debug
};

class Logger
{
public:
    static void setLevel(LogLevel level);
    static void enable(bool enable);
    static void log(LogLevel level, const QString& msg);
    static void logThread(LogLevel level, const QString& msg);
    static void logComputer(const NvComputer* computer,
                            const DeviceInfo* info = nullptr);
private:
    static void writeErrorLog(const QString& msg);
    static LogLevel s_level;
    static bool s_enabled;
    static QString s_logFilePath;
    static QMutex s_logMutex;
};

#define LOG_ERROR(msg) Logger::log(LogLevel::Error, msg)
#define LOG_WARN(msg)  Logger::log(LogLevel::Warning, msg)
#define LOG_INFO(msg)  Logger::log(LogLevel::Info, msg)
#define LOG_DEBUG(msg) Logger::log(LogLevel::Debug, msg)
#define LOG_ERROR_T(msg) Logger::logThread(LogLevel::Error, msg)
#define LOG_WARN_T(msg)  Logger::logThread(LogLevel::Warning, msg)
#define LOG_INFO_T(msg)  Logger::logThread(LogLevel::Info, msg)
#define LOG_DEBUG_T(msg) Logger::logThread(LogLevel::Debug, msg)

#endif // LOGGER_H
