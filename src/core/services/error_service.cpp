#include "error_service.h"
#include "logging_service.h"

ErrorService* ErrorService::s_instance = nullptr;

ErrorService::ErrorService(QObject* parent)
    : QObject(parent)
{
    s_instance = this;
    LoggingService::logInfo("ErrorService", "Initialized");
}

ErrorService& ErrorService::instance()
{
    static ErrorService instance;
    return instance;
}

void ErrorService::reportError(const QString& message, const QString& code, const QString& context)
{
    if (message.isEmpty()) {
        LoggingService::logWarning("ErrorService", "reportError called with empty message");
        return;
    }

    m_lastError = message;
    m_lastErrorCode = code;
    m_lastErrorContext = context;

    LoggingService::logError("ErrorService", QString("Error reported - Context: %1, Code: %2, Message: %3").arg(context, code, message));

    emit errorOccurred(message, code, context);
    emit lastErrorChanged();
    emit hasErrorChanged();
}

void ErrorService::clearError()
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

void ErrorService::report(const QString& message, const QString& code, const QString& context)
{
    auto service = ServiceRegistry::instance().resolve<ErrorService>();
    if (service) {
        service->reportError(message, code, context);
    } else {
        LoggingService::logError("ErrorService", QString("Error reported but service not available: %1").arg(message));
    }
}

