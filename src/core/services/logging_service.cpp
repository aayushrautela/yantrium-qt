#include "logging_service.h"
#include <QDebug>
#include <QDateTime>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(yantriumLog, "yantrium.log")

LoggingService* LoggingService::s_instance = nullptr;

LoggingService::LoggingService(QObject* parent)
    : QObject(parent)
    , m_minLevel(Debug)
{
    s_instance = this;
    qCDebug(yantriumLog) << "[LoggingService] Initialized";
}

LoggingService& LoggingService::instance()
{
    static LoggingService instance;
    return instance;
}

void LoggingService::setMinLevel(LogLevel level)
{
    if (m_minLevel != level) {
        m_minLevel = level;
        qCDebug(yantriumLog) << "[LoggingService] Minimum log level set to:" << levelToString(level);
    }
}

void LoggingService::debug(const QString& category, const QString& message)
{
    log(Debug, category, message);
}

void LoggingService::info(const QString& category, const QString& message)
{
    log(Info, category, message);
}

void LoggingService::warning(const QString& category, const QString& message)
{
    log(Warning, category, message);
}

void LoggingService::error(const QString& category, const QString& message)
{
    log(Error, category, message);
}

void LoggingService::critical(const QString& category, const QString& message)
{
    log(Critical, category, message);
}

void LoggingService::logDebug(const QString& category, const QString& message)
{
    instance().log(Debug, category, message);
}

void LoggingService::logInfo(const QString& category, const QString& message)
{
    instance().log(Info, category, message);
}

void LoggingService::logWarning(const QString& category, const QString& message)
{
    instance().log(Warning, category, message);
}

void LoggingService::logError(const QString& category, const QString& message)
{
    instance().log(Error, category, message);
}

void LoggingService::logCritical(const QString& category, const QString& message)
{
    instance().log(Critical, category, message);
}

QString LoggingService::formatMessage(const QString& format, const QStringList& args)
{
    QString result = format;
    for (int i = 0; i < args.size(); ++i) {
        result.replace(QString("%%1").arg(i + 1), args[i]);
    }
    return result;
}

void LoggingService::log(LogLevel level, const QString& category, const QString& message)
{
    if (level < m_minLevel) {
        return;
    }

    QString formatted = formatLogMessage(level, category, message);
    emit messageLogged(level, category, message);

    // Output to Qt logging system
    switch (level) {
        case Debug:
            qCDebug(yantriumLog) << formatted;
            break;
        case Info:
            qCInfo(yantriumLog) << formatted;
            break;
        case Warning:
            qCWarning(yantriumLog) << formatted;
            break;
        case Error:
            qCCritical(yantriumLog) << formatted;
            break;
        case Critical:
            qCCritical(yantriumLog) << formatted;
            break;
    }
}

QString LoggingService::formatLogMessage(LogLevel level, const QString& category, const QString& message) const
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString levelStr = levelToString(level);
    return QString("[%1] [%2] [%3] %4").arg(timestamp, category, levelStr, message);
}

QString LoggingService::levelToString(LogLevel level) const
{
    switch (level) {
        case Debug: return "DEBUG";
        case Info: return "INFO";
        case Warning: return "WARN";
        case Error: return "ERROR";
        case Critical: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

