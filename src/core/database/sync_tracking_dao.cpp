#include "sync_tracking_dao.h"
#include "database_manager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <utility>

// Modern constructor implementation
SyncTrackingDao::SyncTrackingDao() noexcept = default;

bool SyncTrackingDao::upsertSyncTracking(std::string_view syncType, const QDateTime& lastSyncAt, bool fullSyncCompleted)
{
    const QString syncTypeStr = QString::fromUtf8(syncType.data(), static_cast<int>(syncType.size()));
    QSqlQuery query(getDatabase());

    // Check if record exists
    SyncTrackingRecord existing = getSyncTracking(syncType);
    QDateTime now = QDateTime::currentDateTime();
    QString nowStr = now.toString(Qt::ISODate);
    QString syncAtStr = lastSyncAt.toString(Qt::ISODate);
    
    if (existing.id > 0) {
        // Update existing record
        query.prepare(R"(
            UPDATE sync_tracking 
            SET last_sync_at = ?, full_sync_completed = ?, updated_at = ?
            WHERE sync_type = ?
        )");
        query.addBindValue(syncAtStr);
        query.addBindValue(fullSyncCompleted ? 1 : 0);
        query.addBindValue(nowStr);
        query.addBindValue(syncTypeStr);
    } else {
        // Insert new record
        query.prepare(R"(
            INSERT INTO sync_tracking (
                sync_type, last_sync_at, full_sync_completed, created_at, updated_at
            ) VALUES (?, ?, ?, ?, ?)
        )");
        query.addBindValue(syncTypeStr);
        query.addBindValue(syncAtStr);
        query.addBindValue(fullSyncCompleted ? 1 : 0);
        query.addBindValue(nowStr);
        query.addBindValue(nowStr);
    }
    
    if (!query.exec()) {
        qWarning() << "Failed to upsert sync tracking:" << query.lastError().text();
        return false;
    }
    
    return true;
}

SyncTrackingRecord SyncTrackingDao::getSyncTracking(std::string_view syncType) const
{
    QSqlQuery query(getDatabase());
    query.prepare("SELECT * FROM sync_tracking WHERE sync_type = ?");
    query.addBindValue(QString::fromUtf8(syncType.data(), static_cast<int>(syncType.size())));
    
    if (!query.exec()) {
        qWarning() << "Failed to get sync tracking:" << query.lastError().text();
        return SyncTrackingRecord();
    }
    
    if (query.next()) {
        return recordFromQuery(query);
    }
    
    return SyncTrackingRecord();
}

QList<SyncTrackingRecord> SyncTrackingDao::getAllSyncTracking()
{
    QList<SyncTrackingRecord> records;
    QSqlQuery query(getDatabase());
    query.prepare("SELECT * FROM sync_tracking ORDER BY updated_at DESC");
    
    if (!query.exec()) {
        qWarning() << "Failed to get all sync tracking:" << query.lastError().text();
        return records;
    }
    
    while (query.next()) {
        records.append(recordFromQuery(query));
    }
    
    return records;
}

bool SyncTrackingDao::deleteSyncTracking(std::string_view syncType)
{
    QSqlQuery query(getDatabase());
    query.prepare("DELETE FROM sync_tracking WHERE sync_type = ?");
    query.addBindValue(QString::fromUtf8(syncType.data(), static_cast<int>(syncType.size())));
    
    if (!query.exec()) {
        qWarning() << "Failed to delete sync tracking:" << query.lastError().text();
        return false;
    }
    
    return true;
}

// Modern implementation with manual assignment for structs with constructors
SyncTrackingRecord SyncTrackingDao::recordFromQuery(const QSqlQuery& query) const noexcept
{
    SyncTrackingRecord record;
    record.id = query.value("id").toInt();
    record.syncType = query.value("sync_type").toString();
    record.lastSyncAt = QDateTime::fromString(query.value("last_sync_at").toString(), Qt::ISODate);
    record.fullSyncCompleted = query.value("full_sync_completed").toInt() != 0;
    record.createdAt = QDateTime::fromString(query.value("created_at").toString(), Qt::ISODate);
    record.updatedAt = QDateTime::fromString(query.value("updated_at").toString(), Qt::ISODate);
    return record;
}

