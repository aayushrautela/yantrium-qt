#ifndef WATCH_HISTORY_DAO_H
#define WATCH_HISTORY_DAO_H

#include <QSqlDatabase>
#include <QString>
#include <QDateTime>
#include <QList>
#include <memory>

struct WatchHistoryRecord
{
    int id;
    QString contentId;
    QString type;
    QString title;
    int year;
    QString posterUrl;
    int season;  // For episodes
    int episode;  // For episodes
    QString episodeTitle;  // For episodes
    QDateTime watchedAt;
    double progress;  // 0-100 for episodes
    QString tmdbId;
    QString imdbId;
};

class WatchHistoryDao
{
public:
    explicit WatchHistoryDao();
    
    bool insertWatchHistory(const WatchHistoryRecord& item);
    bool upsertWatchHistory(const WatchHistoryRecord& item);
    QList<WatchHistoryRecord> getWatchHistory(int limit = 100);
    QList<WatchHistoryRecord> getWatchHistoryByContentId(const QString& contentId);
    QList<WatchHistoryRecord> getWatchHistoryForContent(const QString& contentId, const QString& type);
    QList<WatchHistoryRecord> getWatchHistoryByTmdbId(const QString& tmdbId, const QString& type);
    QList<WatchHistoryRecord> getWatchHistoryByContentAndDate(const QString& contentId, const QString& type, const QDateTime& watchedAt);
    bool clearWatchHistory();
    bool removeWatchHistory(const QString& contentId);
    
private:
    QSqlDatabase m_database;
    
    WatchHistoryRecord recordFromQuery(const QSqlQuery& query);
};

#endif // WATCH_HISTORY_DAO_H

