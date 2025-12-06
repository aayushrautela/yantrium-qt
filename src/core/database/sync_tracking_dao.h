#ifndef SYNC_TRACKING_DAO_H
#define SYNC_TRACKING_DAO_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QDateTime>
#include <QList>
#include <string_view>

#include "database_manager.h"

struct SyncTrackingRecord
{
    int id = 0;
    QString syncType;
    QDateTime lastSyncAt;
    bool fullSyncCompleted = false;
    QDateTime createdAt;
    QDateTime updatedAt;

    // Default constructor with modern initialization
    SyncTrackingRecord() = default;

    // Constructor for new sync records
    SyncTrackingRecord(QString syncType, QDateTime lastSyncAt,
                      bool fullSyncCompleted = false)
        : syncType(std::move(syncType)), lastSyncAt(lastSyncAt),
          fullSyncCompleted(fullSyncCompleted) {}

    // Check if record is valid (has been loaded from database)
    [[nodiscard]] bool isValid() const noexcept { return id > 0; }
};

class SyncTrackingDao
{
public:
    // Modern constructor - explicit and noexcept
    explicit SyncTrackingDao() noexcept;

    // Critical sync operations - mark as [[nodiscard]]
    [[nodiscard]] bool upsertSyncTracking(std::string_view syncType, const QDateTime& lastSyncAt, bool fullSyncCompleted = true);
    [[nodiscard]] SyncTrackingRecord getSyncTracking(std::string_view syncType) const;
    [[nodiscard]] QList<SyncTrackingRecord> getAllSyncTracking();
    [[nodiscard]] bool deleteSyncTracking(std::string_view syncType);

private:
    // Helper method - const and noexcept where safe
    [[nodiscard]] SyncTrackingRecord recordFromQuery(const QSqlQuery& query) const noexcept;

    // Database connection getter - modern approach (replaces member variable)
    [[nodiscard]] static QSqlDatabase getDatabase() noexcept {
        return QSqlDatabase::database(DatabaseManager::CONNECTION_NAME);
    }
};

#endif // SYNC_TRACKING_DAO_H


