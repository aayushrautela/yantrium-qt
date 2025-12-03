#include "watch_history_dao.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

WatchHistoryDao::WatchHistoryDao(QSqlDatabase database)
    : m_database(database)
{
}

bool WatchHistoryDao::insertWatchHistory(const WatchHistoryRecord& item)
{
    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT INTO watch_history (
            contentId, type, title, year, posterUrl, season, episode,
            episodeTitle, watchedAt, progress, tmdbId, imdbId
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    
    query.addBindValue(item.contentId);
    query.addBindValue(item.type);
    query.addBindValue(item.title);
    query.addBindValue(item.year);
    query.addBindValue(item.posterUrl.isEmpty() ? QVariant() : QVariant(item.posterUrl));
    query.addBindValue(item.season);
    query.addBindValue(item.episode);
    query.addBindValue(item.episodeTitle.isEmpty() ? QVariant() : QVariant(item.episodeTitle));
    query.addBindValue(item.watchedAt.toString(Qt::ISODate));
    query.addBindValue(item.progress);
    query.addBindValue(item.tmdbId.isEmpty() ? QVariant() : QVariant(item.tmdbId));
    query.addBindValue(item.imdbId.isEmpty() ? QVariant() : QVariant(item.imdbId));
    
    if (!query.exec()) {
        qWarning() << "Failed to insert watch history:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool WatchHistoryDao::upsertWatchHistory(const WatchHistoryRecord& item)
{
    // Check if record already exists with same contentId, type, and watchedAt
    QList<WatchHistoryRecord> existing = getWatchHistoryByContentAndDate(item.contentId, item.type, item.watchedAt);
    
    if (!existing.isEmpty()) {
        // Record already exists, skip insert
        qDebug() << "[WatchHistoryDao] Record already exists, skipping:" << item.title 
                 << "type:" << item.type << "watchedAt:" << item.watchedAt.toString();
        return true;
    }
    
    // Insert new record
    bool success = insertWatchHistory(item);
    if (success) {
        qDebug() << "[WatchHistoryDao] Successfully inserted watch history:" << item.title 
                 << "type:" << item.type << "contentId:" << item.contentId;
    } else {
        qWarning() << "[WatchHistoryDao] Failed to insert watch history:" << item.title;
    }
    return success;
}

QList<WatchHistoryRecord> WatchHistoryDao::getWatchHistory(int limit)
{
    QList<WatchHistoryRecord> items;
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM watch_history ORDER BY watchedAt DESC LIMIT ?");
    query.addBindValue(limit);
    
    if (!query.exec()) {
        qWarning() << "Failed to get watch history:" << query.lastError().text();
        return items;
    }
    
    while (query.next()) {
        items.append(recordFromQuery(query));
    }
    
    return items;
}

QList<WatchHistoryRecord> WatchHistoryDao::getWatchHistoryByContentId(const QString& contentId)
{
    QList<WatchHistoryRecord> items;
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM watch_history WHERE contentId = ? ORDER BY watchedAt DESC");
    query.addBindValue(contentId);
    
    if (!query.exec()) {
        qWarning() << "Failed to get watch history by contentId:" << query.lastError().text();
        return items;
    }
    
    while (query.next()) {
        items.append(recordFromQuery(query));
    }
    
    return items;
}

QList<WatchHistoryRecord> WatchHistoryDao::getWatchHistoryForContent(const QString& contentId, const QString& type)
{
    QList<WatchHistoryRecord> items;
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM watch_history WHERE contentId = ? AND type = ? ORDER BY watchedAt DESC");
    query.addBindValue(contentId);
    query.addBindValue(type);

    if (!query.exec()) {
        qWarning() << "Failed to get watch history for content:" << query.lastError().text();
        return items;
    }

    while (query.next()) {
        items.append(recordFromQuery(query));
    }

    return items;
}

QList<WatchHistoryRecord> WatchHistoryDao::getWatchHistoryByTmdbId(const QString& tmdbId, const QString& type)
{
    QList<WatchHistoryRecord> items;
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM watch_history WHERE tmdbId = ? AND type = ? ORDER BY watchedAt DESC");
    query.addBindValue(tmdbId);
    query.addBindValue(type);

    if (!query.exec()) {
        qWarning() << "Failed to get watch history by tmdbId:" << query.lastError().text();
        return items;
    }

    while (query.next()) {
        items.append(recordFromQuery(query));
    }

    return items;
}

QList<WatchHistoryRecord> WatchHistoryDao::getWatchHistoryByContentAndDate(const QString& contentId, const QString& type, const QDateTime& watchedAt)
{
    QList<WatchHistoryRecord> items;
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM watch_history WHERE contentId = ? AND type = ? AND watchedAt = ? ORDER BY watchedAt DESC");
    query.addBindValue(contentId);
    query.addBindValue(type);
    query.addBindValue(watchedAt.toString(Qt::ISODate));
    
    if (!query.exec()) {
        qWarning() << "Failed to get watch history by content and date:" << query.lastError().text();
        return items;
    }

    while (query.next()) {
        items.append(recordFromQuery(query));
    }

    return items;
}

bool WatchHistoryDao::clearWatchHistory()
{
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM watch_history");
    
    if (!query.exec()) {
        qWarning() << "Failed to clear watch history:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool WatchHistoryDao::removeWatchHistory(const QString& contentId)
{
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM watch_history WHERE contentId = ?");
    query.addBindValue(contentId);
    
    if (!query.exec()) {
        qWarning() << "Failed to remove watch history:" << query.lastError().text();
        return false;
    }
    
    return true;
}

WatchHistoryRecord WatchHistoryDao::recordFromQuery(const QSqlQuery& query)
{
    WatchHistoryRecord record;
    record.id = query.value("id").toInt();
    record.contentId = query.value("contentId").toString();
    record.type = query.value("type").toString();
    record.title = query.value("title").toString();
    record.year = query.value("year").toInt();
    record.posterUrl = query.value("posterUrl").toString();
    record.season = query.value("season").toInt();
    record.episode = query.value("episode").toInt();
    record.episodeTitle = query.value("episodeTitle").toString();
    record.watchedAt = QDateTime::fromString(query.value("watchedAt").toString(), Qt::ISODate);
    record.progress = query.value("progress").toDouble();
    record.tmdbId = query.value("tmdbId").toString();
    record.imdbId = query.value("imdbId").toString();
    return record;
}

