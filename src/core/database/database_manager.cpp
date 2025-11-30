#include "database_manager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QSqlDriver>
#include <QDebug>

DatabaseManager::DatabaseManager(QObject* parent)
    : QObject(parent)
    , m_initialized(false)
{
}

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager instance;
    return instance;
}

bool DatabaseManager::initialize(const QString& databasePath)
{
    if (m_initialized) {
        return true;
    }
    
    // Check if SQLite driver is available
    if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
        qCritical() << "SQLite driver not available!";
        return false;
    }
    
    // Determine database path
    if (databasePath.isEmpty()) {
        QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir dir;
        if (!dir.exists(dataDir)) {
            if (!dir.mkpath(dataDir)) {
                qCritical() << "Failed to create data directory:" << dataDir;
                return false;
            }
        }
        m_databasePath = dataDir + "/yantrium.db";
        qDebug() << "Database path:" << m_databasePath;
    } else {
        m_databasePath = databasePath;
    }
    
    // Open SQLite database
    m_database = QSqlDatabase::addDatabase("QSQLITE", "yantrium");
    m_database.setDatabaseName(m_databasePath);
    
    if (!m_database.open()) {
        qCritical() << "Failed to open database:" << m_database.lastError().text();
        qCritical() << "Database path:" << m_databasePath;
        return false;
    }
    
    qDebug() << "Database opened successfully. Driver:" << m_database.driverName();
    
    qDebug() << "Database opened successfully:" << m_databasePath;
    
    // Create tables
    if (!createAddonsTable()) {
        qWarning() << "Failed to create addons table";
        m_database.close();
        return false;
    }
    
    m_initialized = true;
    return true;
}

QSqlDatabase DatabaseManager::database() const
{
    return m_database;
}

bool DatabaseManager::createAddonsTable()
{
    QSqlQuery query(m_database);
    
    QString createTableSql = R"(
        CREATE TABLE IF NOT EXISTS addons (
            id TEXT PRIMARY KEY,
            name TEXT NOT NULL,
            version TEXT NOT NULL,
            description TEXT,
            manifestUrl TEXT NOT NULL,
            baseUrl TEXT NOT NULL,
            enabled INTEGER NOT NULL DEFAULT 1,
            manifestData TEXT NOT NULL,
            resources TEXT NOT NULL,
            types TEXT NOT NULL,
            createdAt TEXT NOT NULL,
            updatedAt TEXT NOT NULL
        )
    )";
    
    if (!query.exec(createTableSql)) {
        qWarning() << "Failed to create addons table:" << query.lastError().text();
        return false;
    }
    
    qDebug() << "Addons table created successfully";
    return true;
}

