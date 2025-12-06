#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include <QObject>
#include <QString>
#include <memory>
#include <QtQmlIntegration/qqmlintegration.h>
#include "core/di/service_registry.h"

class DatabaseManager;

/**
 * @brief Main application controller that coordinates services and exposes application state
 * 
 * Acts as the central coordinator for the application, using the service registry
 * to resolve dependencies and providing a unified interface for QML.
 */
class AppController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    
    Q_PROPERTY(bool isInitialized READ isInitialized NOTIFY isInitializedChanged)
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    
public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController() override = default;
    
    /**
     * @brief Initialize the application and all services
     * @return True if initialization succeeded, false otherwise
     */
    [[nodiscard]] bool initialize();
    
    /**
     * @brief Check if the application is initialized
     */
    [[nodiscard]] bool isInitialized() const { return m_isInitialized; }
    
    /**
     * @brief Get the application version
     */
    [[nodiscard]] QString appVersion() const { return QStringLiteral("0.1"); }
    
    /**
     * @brief Get the service registry instance
     */
    static ServiceRegistry& serviceRegistry() { return ServiceRegistry::instance(); }
    
signals:
    void isInitializedChanged();
    void initializationFailed(const QString& error);
    
public slots:
    /**
     * @brief Shutdown the application gracefully
     */
    void shutdown();
    
private:
    bool initializeDatabase();
    bool initializeServices();
    
    bool m_isInitialized = false;
};

#endif // APP_CONTROLLER_H

