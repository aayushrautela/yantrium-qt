#include "database_manager.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

// Define the connection name constant
const QString DatabaseManager::CONNECTION_NAME = "yantrium_connection";

DatabaseManager::DatabaseManager(QObject* parent)
    : QObject(parent)
    , m_initialized(false)
{
}

DatabaseManager::~DatabaseManager()
{
    // Proper cleanup: Remove the connection when the app closes
    if (QSqlDatabase::contains(CONNECTION_NAME)) {
        QSqlDatabase::removeDatabase(CONNECTION_NAME);
    }
}


bool DatabaseManager::initialize(const QString& databasePath)
{
    if (m_initialized) {
        return true;
    }

    // 1. Check Driver
    if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
        qCritical() << "SQLite driver not available!";
        return false;
    }

    // 2. Determine Path
    if (databasePath.isEmpty()) {
        QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir dir(dataDir);
        
        if (!dir.exists()) {
            if (!dir.mkpath(".")) {
                qCritical() << "Failed to create data directory:" << dataDir;
                return false;
            }
        }
        m_databasePath = dir.filePath("yantrium.db");
    } else {
        m_databasePath = databasePath;
    }

    qDebug() << "Database path:" << m_databasePath;

    // 3. Initialize Connection
    // We add the database, but we do NOT store the object as a class member.
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", CONNECTION_NAME);
    db.setDatabaseName(m_databasePath);

    if (!db.open()) {
        qCritical() << "Failed to open database:" << db.lastError().text();
        return false;
    }

    qDebug() << "Database opened successfully. Driver:" << db.driverName();

    // 4. Create Tables
    if (!createTables()) {
        qCritical() << "Failed to initialize database schema.";
        db.close();
        return false;
    }

    m_initialized = true;
    return true;
}

bool DatabaseManager::createTables()
{
    // Retrieve the active connection
    QSqlDatabase db = QSqlDatabase::database(CONNECTION_NAME);
    
    // Start a transaction. This is safer and faster.
    if (!db.transaction()) {
        qWarning() << "Failed to start transaction for table creation.";
    }

    QSqlQuery query(db);

    // We define a simple structure to hold table names and their creation SQL
    struct TableSchema {
        const char* name;
        const char* sql;
    };

    // All your table definitions in one clean list
    const TableSchema tables[] = {
        {
            "addons",
            R"(CREATE TABLE IF NOT EXISTS addons (
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
            ))"
        },
        {
            "trakt_auth",
            R"(CREATE TABLE IF NOT EXISTS trakt_auth (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                accessToken TEXT NOT NULL,
                refreshToken TEXT NOT NULL,
                expiresIn INTEGER NOT NULL,
                createdAt TEXT NOT NULL,
                expiresAt TEXT NOT NULL,
                username TEXT,
                slug TEXT
            ))"
        },
        {
            "catalog_preferences",
            R"(CREATE TABLE IF NOT EXISTS catalog_preferences (
                addon_id TEXT NOT NULL,
                catalog_type TEXT NOT NULL,
                catalog_id TEXT NOT NULL DEFAULT '',
                enabled INTEGER NOT NULL DEFAULT 1,
                is_hero_source INTEGER NOT NULL DEFAULT 0,
                created_at TEXT NOT NULL,
                updated_at TEXT NOT NULL,
                "order" INTEGER NOT NULL DEFAULT 0,
                PRIMARY KEY (addon_id, catalog_type, catalog_id)
            ))"
        },
        {
            "local_library",
            R"(CREATE TABLE IF NOT EXISTS local_library (
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
            ))"
        },
        {
            "watch_history",
            R"(CREATE TABLE IF NOT EXISTS watch_history (
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
                imdbId TEXT,
                tvdbId TEXT,
                traktId TEXT
            ))"
        },
        {
            "sync_tracking",
            R"(CREATE TABLE IF NOT EXISTS sync_tracking (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                sync_type TEXT NOT NULL UNIQUE,
                last_sync_at TEXT NOT NULL,
                full_sync_completed INTEGER DEFAULT 0,
                created_at TEXT NOT NULL,
                updated_at TEXT NOT NULL
            ))"
        }
    };

    bool allSuccess = true;

    // Iterate through the list and create tables
    for (const auto& table : tables) {
        if (!query.exec(table.sql)) {
            qCritical() << "Failed to create table" << table.name << ":" << query.lastError().text();
            allSuccess = false;
            break; // Stop immediately on error
        }
        qDebug() << "Table ensured:" << table.name;
    }

    if (allSuccess) {
        db.commit(); // Save everything
        qDebug() << "All tables initialized successfully.";
        return true;
    } else {
        db.rollback(); // Undo everything if something failed
        return false;
    }
}