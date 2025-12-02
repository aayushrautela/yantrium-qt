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
    
    if (!createTraktAuthTable()) {
        qWarning() << "Failed to create trakt_auth table";
        m_database.close();
        return false;
    }
    
    if (!createCatalogPreferencesTable()) {
        qWarning() << "Failed to create catalog_preferences table";
        m_database.close();
        return false;
    }
    
    if (!createLocalLibraryTable()) {
        qWarning() << "Failed to create local_library table";
        m_database.close();
        return false;
    }
    
    if (!createWatchHistoryTable()) {
        qWarning() << "Failed to create watch_history table";
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

bool DatabaseManager::createTraktAuthTable()
{
    QSqlQuery query(m_database);
    
    QString createTableSql = R"(
        CREATE TABLE IF NOT EXISTS trakt_auth (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            accessToken TEXT NOT NULL,
            refreshToken TEXT NOT NULL,
            expiresIn INTEGER NOT NULL,
            createdAt TEXT NOT NULL,
            expiresAt TEXT NOT NULL,
            username TEXT,
            slug TEXT
        )
    )";
    
    if (!query.exec(createTableSql)) {
        qWarning() << "Failed to create trakt_auth table:" << query.lastError().text();
        return false;
    }
    
    qDebug() << "Trakt auth table created successfully";
    return true;
}

bool DatabaseManager::createCatalogPreferencesTable()
{
    QSqlQuery query(m_database);
    
    // Use empty string instead of NULL for catalog_id to avoid PRIMARY KEY issues
    QString createTableSql = R"(
        CREATE TABLE IF NOT EXISTS catalog_preferences (
            addon_id TEXT NOT NULL,
            catalog_type TEXT NOT NULL,
            catalog_id TEXT NOT NULL DEFAULT '',
            enabled INTEGER NOT NULL DEFAULT 1,
            is_hero_source INTEGER NOT NULL DEFAULT 0,
            created_at TEXT NOT NULL,
            updated_at TEXT NOT NULL,
            PRIMARY KEY (addon_id, catalog_type, catalog_id)
        )
    )";
    
    if (!query.exec(createTableSql)) {
        qWarning() << "Failed to create catalog_preferences table:" << query.lastError().text();
        return false;
    }
    
    qDebug() << "Catalog preferences table created successfully";
    return true;
}

bool DatabaseManager::createLocalLibraryTable()
{
    QSqlQuery query(m_database);
    
    QString createTableSql = R"(
        CREATE TABLE IF NOT EXISTS local_library (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            contentId TEXT NOT NULL UNIQUE,
            type TEXT NOT NULL,
            title TEXT NOT NULL,
            year INTEGER,
            posterUrl TEXT,
            backdropUrl TEXT,
            logoUrl TEXT,
            description TEXT,
            rating TEXT,
            addedAt TEXT NOT NULL,
            tmdbId TEXT,
            imdbId TEXT
        )
    )";
    
    if (!query.exec(createTableSql)) {
        qWarning() << "Failed to create local_library table:" << query.lastError().text();
        return false;
    }
    
    qDebug() << "Local library table created successfully";
    return true;
}

bool DatabaseManager::createWatchHistoryTable()
{
    QSqlQuery query(m_database);
    
    QString createTableSql = R"(
        CREATE TABLE IF NOT EXISTS watch_history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            contentId TEXT NOT NULL,
            type TEXT NOT NULL,
            title TEXT NOT NULL,
            year INTEGER,
            posterUrl TEXT,
            season INTEGER,
            episode INTEGER,
            episodeTitle TEXT,
            watchedAt TEXT NOT NULL,
            progress REAL DEFAULT 0,
            tmdbId TEXT,
            imdbId TEXT
        )
    )";
    
    if (!query.exec(createTableSql)) {
        qWarning() << "Failed to create watch_history table:" << query.lastError().text();
        return false;
    }
    
    qDebug() << "Watch history table created successfully";
    return true;
}

