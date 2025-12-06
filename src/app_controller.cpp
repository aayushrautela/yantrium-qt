#include "app_controller.h"
#include "core/database/database_manager.h"
#include <QDebug>

AppController::AppController(QObject *parent)
    : QObject(parent)
{
}

bool AppController::initialize()
{
    if (m_isInitialized) {
        qWarning() << "[AppController] Already initialized";
        return true;
    }
    
    qDebug() << "[AppController] Initializing application...";
    
    // Initialize database first
    if (!initializeDatabase()) {
        emit initializationFailed("Failed to initialize database");
        return false;
    }
    
    // Initialize services
    if (!initializeServices()) {
        emit initializationFailed("Failed to initialize services");
        return false;
    }
    
    m_isInitialized = true;
    emit isInitializedChanged();
    qDebug() << "[AppController] Application initialized successfully";
    
    return true;
}

bool AppController::initializeDatabase()
{
    DatabaseManager& dbManager = DatabaseManager::instance();
    if (!dbManager.initialize()) {
        qCritical() << "[AppController] Failed to initialize database";
        return false;
    }
    qDebug() << "[AppController] Database initialized";
    return true;
}

bool AppController::initializeServices()
{
    // Services will be registered in main.cpp after this controller is created
    // This method can be extended to perform service-specific initialization
    qDebug() << "[AppController] Services will be initialized by main.cpp";
    return true;
}

void AppController::shutdown()
{
    if (!m_isInitialized) {
        return;
    }
    
    qDebug() << "[AppController] Shutting down application...";
    
    // Clear service registry
    ServiceRegistry::instance().clear();
    
    m_isInitialized = false;
    emit isInitializedChanged();
    
    qDebug() << "[AppController] Application shutdown complete";
}

