#ifndef LOGGING_SERVICE_H
#define LOGGING_SERVICE_H

#include <QObject>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

/**
 * @brief Centralized logging service with consistent format and levels
 * 
 * Provides standardized logging across C++ and QML with consistent formatting.
 * All log messages follow the format: [Timestamp] [Category] [Level] Message
 */
class LoggingService : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    /**
     * @brief Log levels
     */
    enum LogLevel {
        Debug = 0,
        Info = 1,
        Warning = 2,
        Error = 3,
        Critical = 4
    };
    Q_ENUM(LogLevel)

    explicit LoggingService(QObject* parent = nullptr);

    /**
     * @brief Get singleton instance (for C++ usage)
     */
    static LoggingService& instance();

    /**
     * @brief Set minimum log level (messages below this level are ignored)
     */
    Q_INVOKABLE void setMinLevel(LogLevel level);
    Q_INVOKABLE LogLevel minLevel() const { return m_minLevel; }

    /**
     * @brief Log a debug message
     */
    Q_INVOKABLE void debug(const QString& category, const QString& message);
    
    /**
     * @brief Log an info message
     */
    Q_INVOKABLE void info(const QString& category, const QString& message);
    
    /**
     * @brief Log a warning message
     */
    Q_INVOKABLE void warning(const QString& category, const QString& message);
    
    /**
     * @brief Log an error message
     */
    Q_INVOKABLE void error(const QString& category, const QString& message);
    
    /**
     * @brief Log a critical message
     */
    Q_INVOKABLE void critical(const QString& category, const QString& message);

    /**
     * @brief C++ convenience methods (with formatting support)
     */
    static void logDebug(const QString& category, const QString& message);
    static void logInfo(const QString& category, const QString& message);
    static void logWarning(const QString& category, const QString& message);
    static void logError(const QString& category, const QString& message);
    static void logCritical(const QString& category, const QString& message);

    /**
     * @brief Format message with arguments (C++ only)
     */
    static QString formatMessage(const QString& format, const QStringList& args);

signals:
    /**
     * @brief Emitted when a log message is generated
     */
    void messageLogged(LogLevel level, const QString& category, const QString& message);

private:
    void log(LogLevel level, const QString& category, const QString& message);
    QString formatLogMessage(LogLevel level, const QString& category, const QString& message) const;
    QString levelToString(LogLevel level) const;

    LogLevel m_minLevel = Debug;
    static LoggingService* s_instance;
};

#endif // LOGGING_SERVICE_H

