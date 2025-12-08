#ifndef ERROR_SERVICE_H
#define ERROR_SERVICE_H

#include <QObject>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

/**
 * @brief Centralized error handling service
 * 
 * Provides standardized error reporting and display coordination.
 * All services should emit errors through this service for consistent handling.
 */
class ErrorService : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY hasErrorChanged)

public:
    explicit ErrorService(QObject* parent = nullptr);

    /**
     * @brief Get the last error message
     */
    [[nodiscard]] QString lastError() const { return m_lastError; }

    /**
     * @brief Check if there is a current error
     */
    [[nodiscard]] bool hasError() const { return !m_lastError.isEmpty(); }

    /**
     * @brief Report an error
     * @param message Error message
     * @param code Optional error code
     * @param context Optional context (service name, etc.)
     */
    Q_INVOKABLE void reportError(const QString& message, const QString& code = "", const QString& context = "");

    /**
     * @brief C++ convenience method for reporting errors
     */
    static void report(const QString& message, const QString& code = "", const QString& context = "");

    /**
     * @brief Clear the current error
     */
    Q_INVOKABLE void clearError();

signals:
    /**
     * @brief Emitted when an error occurs
     * @param message Error message
     * @param code Error code (if provided)
     * @param context Error context (if provided)
     */
    void errorOccurred(const QString& message, const QString& code, const QString& context);

    /**
     * @brief Emitted when the last error changes
     */
    void lastErrorChanged();

    /**
     * @brief Emitted when error state changes
     */
    void hasErrorChanged();

private:
    QString m_lastError;
    QString m_lastErrorCode;
    QString m_lastErrorContext;
};

#endif // ERROR_SERVICE_H

