#include "local_library_service.h"
#include "../database/database_manager.h"
#include <QDebug>
#include <QDateTime>

LocalLibraryService::LocalLibraryService(QObject* parent)
    : QObject(parent)
    , m_dbManager(&DatabaseManager::instance())
{
    if (m_dbManager->isInitialized()) {
        m_libraryDao = new LocalLibraryDao(m_dbManager->database());
        m_historyDao = new WatchHistoryDao(m_dbManager->database());
    } else {
        qWarning() << "[LocalLibraryService] Database not initialized";
        m_libraryDao = nullptr;
        m_historyDao = nullptr;
    }
}

void LocalLibraryService::addToLibrary(const QVariantMap& item)
{
    if (!m_libraryDao) {
        emit error("Database not initialized");
        return;
    }
    
    LocalLibraryRecord record = variantMapToRecord(item);
    record.addedAt = QDateTime::currentDateTime();
    
    bool success = m_libraryDao->insertLibraryItem(record);
    emit libraryItemAdded(success);
    
    if (!success) {
        emit error("Failed to add item to library");
    }
}

void LocalLibraryService::removeFromLibrary(const QString& contentId)
{
    if (!m_libraryDao) {
        emit error("Database not initialized");
        return;
    }
    
    if (contentId.isEmpty()) {
        emit error("Content ID is required");
        return;
    }
    
    bool success = m_libraryDao->removeLibraryItem(contentId);
    emit libraryItemRemoved(success);
    
    if (!success) {
        emit error("Failed to remove item from library");
    }
}

void LocalLibraryService::getLibraryItems()
{
    if (!m_libraryDao) {
        emit error("Database not initialized");
        emit libraryItemsLoaded(QVariantList());
        return;
    }
    
    QList<LocalLibraryRecord> records = m_libraryDao->getAllLibraryItems();
    QVariantList items;
    
    for (const LocalLibraryRecord& record : records) {
        items.append(recordToVariantMap(record));
    }
    
    emit libraryItemsLoaded(items);
}

void LocalLibraryService::isInLibrary(const QString& contentId)
{
    if (!m_libraryDao) {
        emit error("Database not initialized");
        emit isInLibraryResult(false);
        return;
    }
    
    if (contentId.isEmpty()) {
        emit isInLibraryResult(false);
        return;
    }
    
    bool inLibrary = m_libraryDao->isInLibrary(contentId);
    emit isInLibraryResult(inLibrary);
}

void LocalLibraryService::addToWatchHistory(const QVariantMap& item)
{
    if (!m_historyDao) {
        emit error("Database not initialized");
        return;
    }
    
    WatchHistoryRecord record = variantMapToHistoryRecord(item);
    record.watchedAt = QDateTime::currentDateTime();
    
    bool success = m_historyDao->insertWatchHistory(record);
    
    if (!success) {
        emit error("Failed to add item to watch history");
    }
}

void LocalLibraryService::getWatchHistory(int limit)
{
    if (!m_historyDao) {
        emit error("Database not initialized");
        emit watchHistoryLoaded(QVariantList());
        return;
    }
    
    QList<WatchHistoryRecord> records = m_historyDao->getWatchHistory(limit);
    QVariantList items;
    
    for (const WatchHistoryRecord& record : records) {
        items.append(historyRecordToVariantMap(record));
    }
    
    emit watchHistoryLoaded(items);
}

void LocalLibraryService::getWatchProgress(const QString& contentId, const QString& type, int season, int episode)
{
    qDebug() << "[LocalLibraryService] getWatchProgress:" << contentId << type << "S" << season << "E" << episode;

    QVariantMap progress;
    progress["contentId"] = contentId;
    progress["type"] = type;
    progress["hasProgress"] = false;
    progress["progress"] = 0.0;
    progress["lastWatchedSeason"] = -1;
    progress["lastWatchedEpisode"] = -1;
    progress["lastWatchedAt"] = "";
    progress["isWatched"] = false;

    QList<WatchHistoryRecord> records = m_historyDao->getWatchHistoryForContent(contentId, type);

    if (records.isEmpty()) {
        qDebug() << "[LocalLibraryService] No watch history found for:" << contentId;
        emit watchProgressLoaded(progress);
        return;
    }

    progress["hasProgress"] = true;

    if (type == "movie") {
        // For movies, find the most recent watch record
        WatchHistoryRecord latestRecord = records.first();
        for (const WatchHistoryRecord& record : records) {
            if (record.watchedAt > latestRecord.watchedAt) {
                latestRecord = record;
            }
        }

        progress["progress"] = latestRecord.progress;
        progress["lastWatchedAt"] = latestRecord.watchedAt.toString(Qt::ISODate);
        progress["isWatched"] = (latestRecord.progress >= 0.95); // Consider 95% as watched

        qDebug() << "[LocalLibraryService] Movie progress:" << latestRecord.progress << "watched:" << progress["isWatched"];

    } else if (type == "tv" || type == "show") {
        // For TV shows, analyze episode progress
        int maxSeason = -1;
        int maxEpisode = -1;
        double maxProgress = 0.0;
        QDateTime latestWatched;

        // Find the highest watched episode and its progress
        for (const WatchHistoryRecord& record : records) {
            if (record.season > maxSeason ||
                (record.season == maxSeason && record.episode > maxEpisode) ||
                (record.season == maxSeason && record.episode == maxEpisode && record.watchedAt > latestWatched)) {
                maxSeason = record.season;
                maxEpisode = record.episode;
                maxProgress = record.progress;
                latestWatched = record.watchedAt;
            }
        }

        progress["lastWatchedSeason"] = maxSeason;
        progress["lastWatchedEpisode"] = maxEpisode;
        progress["progress"] = maxProgress;
        progress["lastWatchedAt"] = latestWatched.toString(Qt::ISODate);
        progress["isWatched"] = (maxProgress >= 0.95); // Consider 95% as watched

        // If requesting specific episode progress
        if (season != -1 && episode != -1) {
            for (const WatchHistoryRecord& record : records) {
                if (record.season == season && record.episode == episode) {
                    progress["episodeProgress"] = record.progress;
                    progress["episodeWatchedAt"] = record.watchedAt.toString(Qt::ISODate);
                    progress["episodeIsWatched"] = (record.progress >= 0.95);
                    break;
                }
            }
        }

        qDebug() << "[LocalLibraryService] TV progress - last S" << maxSeason << "E" << maxEpisode
                 << "progress:" << maxProgress << "watched:" << progress["isWatched"];
    }

    emit watchProgressLoaded(progress);
}

void LocalLibraryService::getWatchProgressByTmdbId(const QString& tmdbId, const QString& type, int season, int episode)
{
    qWarning() << "[LocalLibraryService] ===== getWatchProgressByTmdbId CALLED =====";
    qWarning() << "[LocalLibraryService] getWatchProgressByTmdbId:" << tmdbId << type << "S" << season << "E" << episode;

    QVariantMap progress;
    progress["contentId"] = tmdbId; // Store tmdbId in contentId field for compatibility
    progress["type"] = type;
    progress["hasProgress"] = false;
    progress["progress"] = 0.0;
    progress["lastWatchedSeason"] = -1;
    progress["lastWatchedEpisode"] = -1;
    progress["lastWatchedAt"] = "";
    progress["isWatched"] = false;

    if (tmdbId.isEmpty()) {
        qDebug() << "[LocalLibraryService] Empty TMDB ID, no watch history";
        emit watchProgressLoaded(progress);
        return;
    }

    // Normalize type for database lookup - database uses "tv", not "series"
    QString dbType = type;
    if (type == "series") {
        dbType = "tv";
    }
    
    qWarning() << "[LocalLibraryService] Looking up watch history - tmdbId:" << tmdbId << "type:" << type << "dbType:" << dbType;
    
    QList<WatchHistoryRecord> records = m_historyDao->getWatchHistoryByTmdbId(tmdbId, dbType);
    qWarning() << "[LocalLibraryService] Database query result - Found" << records.size() << "watch history records for TMDB ID" << tmdbId << "type" << dbType;
    
    if (records.size() > 0) {
        qWarning() << "[LocalLibraryService] First record - title:" << records.first().title 
                   << "progress:" << records.first().progress 
                   << "watchedAt:" << records.first().watchedAt.toString();
    }

    if (records.isEmpty()) {
        qDebug() << "[LocalLibraryService] No watch history found for TMDB ID:" << tmdbId;
        emit watchProgressLoaded(progress);
        return;
    }

    progress["hasProgress"] = true;

    if (type == "movie") {
        // For movies, find the most recent watch record
        WatchHistoryRecord latestRecord = records.first();
        for (const WatchHistoryRecord& record : records) {
            if (record.watchedAt > latestRecord.watchedAt) {
                latestRecord = record;
            }
        }

        progress["progress"] = latestRecord.progress;
        progress["lastWatchedAt"] = latestRecord.watchedAt.toString(Qt::ISODate);
        progress["isWatched"] = (latestRecord.progress >= 0.95); // Consider 95% as watched

        qDebug() << "[LocalLibraryService] Movie progress:" << latestRecord.progress << "watched:" << progress["isWatched"];

    } else if (type == "tv" || type == "show") {
        // For TV shows, analyze episode progress
        int maxSeason = -1;
        int maxEpisode = -1;
        double maxProgress = 0.0;
        QDateTime latestWatched;

        // Find the highest watched episode and its progress
        for (const WatchHistoryRecord& record : records) {
            if (record.season > maxSeason ||
                (record.season == maxSeason && record.episode > maxEpisode) ||
                (record.season == maxSeason && record.episode == maxEpisode && record.watchedAt > latestWatched)) {
                maxSeason = record.season;
                maxEpisode = record.episode;
                maxProgress = record.progress;
                latestWatched = record.watchedAt;
            }
        }

        progress["lastWatchedSeason"] = maxSeason;
        progress["lastWatchedEpisode"] = maxEpisode;
        progress["progress"] = maxProgress;
        progress["lastWatchedAt"] = latestWatched.toString(Qt::ISODate);
        progress["isWatched"] = (maxProgress >= 0.95); // Consider 95% as watched

        // If requesting specific episode progress
        if (season != -1 && episode != -1) {
            for (const WatchHistoryRecord& record : records) {
                if (record.season == season && record.episode == episode) {
                    progress["episodeProgress"] = record.progress;
                    progress["episodeWatchedAt"] = record.watchedAt.toString(Qt::ISODate);
                    progress["episodeIsWatched"] = (record.progress >= 0.95);
                    break;
                }
            }
        }

        qWarning() << "[LocalLibraryService] TV progress - last S" << maxSeason << "E" << maxEpisode
                 << "progress:" << maxProgress << "watched:" << progress["isWatched"];
    }

    qWarning() << "[LocalLibraryService] Emitting watchProgressLoaded - hasProgress:" << progress["hasProgress"] 
               << "isWatched:" << progress["isWatched"] << "progress:" << progress["progress"]
               << "contentId:" << progress["contentId"];
    emit watchProgressLoaded(progress);
    qWarning() << "[LocalLibraryService] ===== getWatchProgressByTmdbId COMPLETE =====";
}

QVariantMap LocalLibraryService::recordToVariantMap(const LocalLibraryRecord& record)
{
    QVariantMap map;
    map["id"] = record.id;
    map["contentId"] = record.contentId;
    map["type"] = record.type;
    map["title"] = record.title;
    map["year"] = record.year;
    map["posterUrl"] = record.posterUrl;
    map["backdropUrl"] = record.backdropUrl;
    map["logoUrl"] = record.logoUrl;
    map["description"] = record.description;
    map["rating"] = record.rating;
    map["addedAt"] = record.addedAt.toString(Qt::ISODate);
    map["tmdbId"] = record.tmdbId;
    map["imdbId"] = record.imdbId;
    return map;
}

LocalLibraryRecord LocalLibraryService::variantMapToRecord(const QVariantMap& map)
{
    LocalLibraryRecord record;
    record.id = map["id"].toInt();
    record.contentId = map["contentId"].toString();
    record.type = map["type"].toString();
    record.title = map["title"].toString();
    record.year = map["year"].toInt();
    record.posterUrl = map["posterUrl"].toString();
    record.backdropUrl = map["backdropUrl"].toString();
    record.logoUrl = map["logoUrl"].toString();
    record.description = map["description"].toString();
    record.rating = map["rating"].toString();
    
    if (map.contains("addedAt") && !map["addedAt"].toString().isEmpty()) {
        record.addedAt = QDateTime::fromString(map["addedAt"].toString(), Qt::ISODate);
    }
    
    record.tmdbId = map["tmdbId"].toString();
    record.imdbId = map["imdbId"].toString();
    return record;
}

QVariantMap LocalLibraryService::historyRecordToVariantMap(const WatchHistoryRecord& record)
{
    QVariantMap map;
    map["id"] = record.id;
    map["contentId"] = record.contentId;
    map["type"] = record.type;
    map["title"] = record.title;
    map["year"] = record.year;
    map["posterUrl"] = record.posterUrl;
    map["season"] = record.season;
    map["episode"] = record.episode;
    map["episodeTitle"] = record.episodeTitle;
    map["watchedAt"] = record.watchedAt.toString(Qt::ISODate);
    map["progress"] = record.progress;
    map["tmdbId"] = record.tmdbId;
    map["imdbId"] = record.imdbId;
    return map;
}

WatchHistoryRecord LocalLibraryService::variantMapToHistoryRecord(const QVariantMap& map)
{
    WatchHistoryRecord record;
    record.id = map["id"].toInt();
    record.contentId = map["contentId"].toString();
    record.type = map["type"].toString();
    record.title = map["title"].toString();
    record.year = map["year"].toInt();
    record.posterUrl = map["posterUrl"].toString();
    record.season = map["season"].toInt();
    record.episode = map["episode"].toInt();
    record.episodeTitle = map["episodeTitle"].toString();
    
    if (map.contains("watchedAt") && !map["watchedAt"].toString().isEmpty()) {
        record.watchedAt = QDateTime::fromString(map["watchedAt"].toString(), Qt::ISODate);
    }
    
    record.progress = map["progress"].toDouble();
    record.tmdbId = map["tmdbId"].toString();
    record.imdbId = map["imdbId"].toString();
    return record;
}

