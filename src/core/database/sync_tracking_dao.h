#ifndef SYNC_TRACKING_DAO_H
#define SYNC_TRACKING_DAO_H

#include <QSqlDatabase>
#include <QString>
#include <QDateTime>
#include <QList>

struct SyncTrackingRecord
{
    int id;
    QString syncType;
    QDateTime lastSyncAt;
    bool fullSyncCompleted;
    QDateTime createdAt;
    QDateTime updatedAt;
};

class SyncTrackingDao
{
public:
    explicit SyncTrackingDao();
    
    bool upsertSyncTracking(const QString& syncType, const QDateTime& lastSyncAt, bool fullSyncCompleted = true);
    SyncTrackingRecord getSyncTracking(const QString& syncType);
    QList<SyncTrackingRecord> getAllSyncTracking();
    bool deleteSyncTracking(const QString& syncType);
    
private:
    QSqlDatabase m_database;
    SyncTrackingRecord recordFromQuery(const QSqlQuery& query);
};

#endif // SYNC_TRACKING_DAO_H


