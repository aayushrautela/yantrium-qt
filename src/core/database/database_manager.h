#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <memory>

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    static DatabaseManager& instance();
    
    bool initialize(const QString& databasePath = QString());
    QSqlDatabase database() const;
    bool isInitialized() const { return m_initialized; }
    
    // Table creation
    bool createAddonsTable();
    bool createTraktAuthTable();
    bool createCatalogPreferencesTable();
    bool createLocalLibraryTable();
    bool createWatchHistoryTable();
    bool createSyncTrackingTable();
    
private:
    DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager() = default;
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;
    
    QSqlDatabase m_database;
    bool m_initialized;
    QString m_databasePath;
};

#endif // DATABASE_MANAGER_H

