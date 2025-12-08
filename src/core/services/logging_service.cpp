#include "logging_service.h"
#include "core/di/service_registry.h"
#include <QDebug>
#include <QDateTime>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(yantriumLog, "yantrium.log")

LoggingService::LoggingService(QObject* parent)
    : QObject(parent)
    , m_minLevel(Debug)
{
    qCDebug(yantriumLog) << "[LoggingService] Initialized";
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
    auto service = ServiceRegistry::instance().resolve<LoggingService>();
    if (service) {
        service->log(Debug, category, message);
    } else {
        qCDebug(yantriumLog) << QString("[%1] [DEBUG] %2").arg(category, message);
    }
}

void LoggingService::logInfo(const QString& category, const QString& message)
{
    auto service = ServiceRegistry::instance().resolve<LoggingService>();
    if (service) {
        service->log(Info, category, message);
    } else {
        qCInfo(yantriumLog) << QString("[%1] [INFO] %2").arg(category, message);
    }
}

void LoggingService::logWarning(const QString& category, const QString& message)
{
    auto service = ServiceRegistry::instance().resolve<LoggingService>();
    if (service) {
        service->log(Warning, category, message);
    } else {
        qCWarning(yantriumLog) << QString("[%1] [WARN] %2").arg(category, message);
    }
}

void LoggingService::logError(const QString& category, const QString& message)
{
    auto service = ServiceRegistry::instance().resolve<LoggingService>();
    if (service) {
        service->log(Error, category, message);
    } else {
        qCCritical(yantriumLog) << QString("[%1] [ERROR] %2").arg(category, message);
    }
}

void LoggingService::logCritical(const QString& category, const QString& message)
{
    auto service = ServiceRegistry::instance().resolve<LoggingService>();
    if (service) {
        service->log(Critical, category, message);
    } else {
        qCCritical(yantriumLog) << QString("[%1] [CRITICAL] %2").arg(category, message);
    }
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

void LoggingService::reportError(const QString& message, const QString& code, const QString& context)
{
    if (message.isEmpty()) {
        logWarning("LoggingService", "reportError called with empty message");
        return;
    }

    m_lastError = message;
    m_lastErrorCode = code;
    m_lastErrorContext = context;

    // Log the error
    QString errorMsg = QString("Error reported - Context: %1, Code: %2, Message: %3").arg(context, code, message);
    log(Error, context.isEmpty() ? "LoggingService" : context, errorMsg);

    emit errorOccurred(message, code, context);
    emit lastErrorChanged();
    emit hasErrorChanged();
}

void LoggingService::clearError()
{
    if (m_lastError.isEmpty()) {
        return;
    }

    m_lastError.clear();
    m_lastErrorCode.clear();
    m_lastErrorContext.clear();

    emit lastErrorChanged();
    emit hasErrorChanged();
}

void LoggingService::report(const QString& message, const QString& code, const QString& context)
{
    auto service = ServiceRegistry::instance().resolve<LoggingService>();
    if (service) {
        service->reportError(message, code, context);
    } else {
        qCCritical(yantriumLog) << QString("[LoggingService] [ERROR] Error reported but service not available: %1").arg(message);
    }
}

