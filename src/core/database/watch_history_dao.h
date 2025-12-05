#ifndef WATCH_HISTORY_DAO_H
#define WATCH_HISTORY_DAO_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QDateTime>
#include <QList>
#include <memory>
#include <string_view>

#include "database_manager.h"

struct WatchHistoryRecord
{
    int id = 0;
    QString contentId;
    QString type;
    QString title;
    int year = 0;
    QString posterUrl;
    int season = 0;  // For episodes
    int episode = 0;  // For episodes
    QString episodeTitle;  // For episodes
    QDateTime watchedAt;
    double progress = 0.0;  // 0-100 for episodes
    QString tmdbId;
    QString imdbId;

    // Default constructor with modern initialization
    WatchHistoryRecord() = default;

    // Constructor for watching a movie/show
    WatchHistoryRecord(QString contentId, QString type, QString title,
                      QDateTime watchedAt, int year = 0)
        : contentId(std::move(contentId)), type(std::move(type)),
          title(std::move(title)), watchedAt(watchedAt), year(year) {}

    // Constructor for watching an episode
    WatchHistoryRecord(QString contentId, QString type, QString title,
                      int season, int episode, QString episodeTitle,
                      QDateTime watchedAt, double progress = 0.0, int year = 0)
        : contentId(std::move(contentId)), type(std::move(type)),
          title(std::move(title)), year(year), season(season), episode(episode),
          episodeTitle(std::move(episodeTitle)), watchedAt(watchedAt),
          progress(progress) {}

    // Check if record is valid (has been loaded from database)
    [[nodiscard]] bool isValid() const noexcept { return id > 0; }

    // Check if this is an episode record
    [[nodiscard]] bool isEpisode() const noexcept { return season > 0 && episode > 0; }

    // Check if this is a movie record
    [[nodiscard]] bool isMovie() const noexcept { return season == 0 && episode == 0; }

    // Get a display title that includes episode info if applicable
    [[nodiscard]] QString getDisplayTitle() const {
        if (isEpisode() && !episodeTitle.isEmpty()) {
            return QString("%1: %2").arg(title, episodeTitle);
        }
        return title;
    }
};

class WatchHistoryDao
{
public:
    // Modern constructor - explicit and noexcept
    explicit WatchHistoryDao() noexcept;

    // Critical watch history operations - mark as [[nodiscard]]
    [[nodiscard]] bool insertWatchHistory(const WatchHistoryRecord& item);
    [[nodiscard]] bool upsertWatchHistory(const WatchHistoryRecord& item);
    [[nodiscard]] QList<WatchHistoryRecord> getWatchHistory(int limit = 100);
    [[nodiscard]] QList<WatchHistoryRecord> getWatchHistoryByContentId(std::string_view contentId);
    [[nodiscard]] QList<WatchHistoryRecord> getWatchHistoryForContent(std::string_view contentId, std::string_view type);
    [[nodiscard]] QList<WatchHistoryRecord> getWatchHistoryByTmdbId(std::string_view tmdbId, std::string_view type);
    [[nodiscard]] QList<WatchHistoryRecord> getWatchHistoryByContentAndDate(std::string_view contentId, std::string_view type, const QDateTime& watchedAt);
    [[nodiscard]] bool clearWatchHistory();
    [[nodiscard]] bool removeWatchHistory(std::string_view contentId);

private:
    // Helper method - const and noexcept where safe
    [[nodiscard]] WatchHistoryRecord recordFromQuery(const QSqlQuery& query) const noexcept;

    // Database connection getter - modern approach (replaces member variable)
    [[nodiscard]] static QSqlDatabase getDatabase() noexcept {
        return QSqlDatabase::database(DatabaseManager::CONNECTION_NAME);
    }
};

#endif // WATCH_HISTORY_DAO_H

