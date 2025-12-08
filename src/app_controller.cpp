#include "app_controller.h"
#include "core/database/database_manager.h"
#include "core/di/service_registry.h"
#include "core/services/logging_service.h"

AppController::AppController(QObject *parent)
    : QObject(parent)
{
}

bool AppController::initialize()
{
    if (m_isInitialized) {
        LoggingService::logWarning("AppController", "Already initialized");
        return true;
    }
    
    LoggingService::logDebug("AppController", "Initializing application...");
    
    // Database will be initialized in main.cpp when DatabaseManager is registered
    // Just verify it's available
    ServiceRegistry& registry = ServiceRegistry::instance();
    auto dbManager = registry.resolve<DatabaseManager>();
    if (!dbManager) {
        LoggingService::logCritical("AppController", "DatabaseManager not available in registry");
        emit initializationFailed("Failed to access database manager");
        return false;
    }
    
    if (!dbManager->isInitialized()) {
        LoggingService::logCritical("AppController", "Database not initialized");
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
    LoggingService::logDebug("AppController", "Application initialized successfully");
    
    return true;
}

bool AppController::initializeDatabase()
{
    // Database initialization is now handled in main.cpp when DatabaseManager is registered
    // This method is kept for compatibility but does nothing
    return true;
}

bool AppController::initializeServices()
{
    // Services will be registered in main.cpp after this controller is created
    // This method can be extended to perform service-specific initialization
    LoggingService::logDebug("AppController", "Services will be initialized by main.cpp");
    return true;
}

void AppController::shutdown()
{
    if (!m_isInitialized) {
        return;
    }
    
    LoggingService::logDebug("AppController", "Shutting down application...");
    
    // Clear service registry
    ServiceRegistry::instance().clear();
    
    m_isInitialized = false;
    emit isInitializedChanged();
    
    LoggingService::logDebug("AppController", "Application shutdown complete");
}

