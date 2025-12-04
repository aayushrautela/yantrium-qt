#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <QObject>
#include <QString>
// We do NOT include QSqlDatabase here to avoid exposing the instance in the header.
// This enforces the practice of retrieving the connection by name.

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    // Thread-safe Singleton access
    static DatabaseManager& instance();

    // Delete copy and move constructors to ensure Singleton uniqueness
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    // Initialization
    bool initialize(const QString& databasePath = QString());
    bool isInitialized() const { return m_initialized; }

    // Helper to get the constant connection name used throughout the app
    static const QString CONNECTION_NAME;

private:
    explicit DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager();

    // Internal helper to create all tables in a single transaction
    bool createTables();

    bool m_initialized;
    QString m_databasePath;
};

#endif // DATABASE_MANAGER_H