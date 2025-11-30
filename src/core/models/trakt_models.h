#ifndef TRAKT_MODELS_H
#define TRAKT_MODELS_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QList>
#include <QMap>
#include <memory>

// Forward declarations
class TraktMovie;
class TraktShow;
class TraktEpisode;

/// Trakt IDs structure
class TraktIds
{
public:
    TraktIds();
    TraktIds(const QString& trakt, const QString& slug, const QString& imdb, const QString& tmdb);
    
    static TraktIds fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    QString trakt() const { return m_trakt; }
    QString slug() const { return m_slug; }
    QString imdb() const { return m_imdb; }
    QString tmdb() const { return m_tmdb; }

private:
    QString m_trakt;
    QString m_slug;
    QString m_imdb;
    QString m_tmdb;
};

/// Trakt Movie
class TraktMovie
{
public:
    TraktMovie();
    TraktMovie(const QString& title, int year, const TraktIds& ids);
    
    static TraktMovie fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    QString title() const { return m_title; }
    int year() const { return m_year; }
    TraktIds ids() const { return m_ids; }

private:
    QString m_title;
    int m_year;
    TraktIds m_ids;
};

/// Trakt Show
class TraktShow
{
public:
    TraktShow();
    TraktShow(const QString& title, int year, const TraktIds& ids);
    
    static TraktShow fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    QString title() const { return m_title; }
    int year() const { return m_year; }
    TraktIds ids() const { return m_ids; }

private:
    QString m_title;
    int m_year;
    TraktIds m_ids;
};

/// Trakt Episode
class TraktEpisode
{
public:
    TraktEpisode();
    TraktEpisode(int season, int number, const QString& title, const TraktIds& ids, int runtime = 0);
    
    static TraktEpisode fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    int season() const { return m_season; }
    int number() const { return m_number; }
    QString title() const { return m_title; }
    TraktIds ids() const { return m_ids; }
    int runtime() const { return m_runtime; }

private:
    int m_season;
    int m_number;
    QString m_title;
    TraktIds m_ids;
    int m_runtime;
};

/// Trakt User
class TraktUser
{
public:
    TraktUser();
    TraktUser(const QString& username, const QString& slug, bool isPrivate, const QString& name,
              bool vip, bool vipEp, const TraktIds& ids);
    
    static TraktUser fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    QString username() const { return m_username; }
    QString slug() const { return m_slug; }
    bool isPrivate() const { return m_private; }
    QString name() const { return m_name; }
    bool vip() const { return m_vip; }
    bool vipEp() const { return m_vipEp; }
    TraktIds ids() const { return m_ids; }

private:
    QString m_username;
    QString m_slug;
    bool m_private;
    QString m_name;
    bool m_vip;
    bool m_vipEp;
    TraktIds m_ids;
};

/// Trakt Images
class TraktImages
{
public:
    TraktImages();
    TraktImages(const QStringList& fanart, const QStringList& poster, const QStringList& logo,
                const QStringList& clearart, const QStringList& banner, const QStringList& thumb);
    
    static TraktImages fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    QStringList fanart() const { return m_fanart; }
    QStringList poster() const { return m_poster; }
    QStringList logo() const { return m_logo; }
    QStringList clearart() const { return m_clearart; }
    QStringList banner() const { return m_banner; }
    QStringList thumb() const { return m_thumb; }
    
    QString getPosterUrl() const;
    QString getFanartUrl() const;

private:
    QStringList m_fanart;
    QStringList m_poster;
    QStringList m_logo;
    QStringList m_clearart;
    QStringList m_banner;
    QStringList m_thumb;
};

/// Trakt Content Data (for scrobbling)
class TraktContentData
{
public:
    TraktContentData();
    TraktContentData(const QString& type, const QString& imdbId, const QString& title, int year,
                     int season = 0, int episode = 0, const QString& showTitle = QString(),
                     int showYear = 0, const QString& showImdbId = QString());
    
    QString getContentKey() const;
    QJsonObject toJson() const;
    
    QString type() const { return m_type; }
    QString imdbId() const { return m_imdbId; }
    QString title() const { return m_title; }
    int year() const { return m_year; }
    int season() const { return m_season; }
    int episode() const { return m_episode; }
    QString showTitle() const { return m_showTitle; }
    int showYear() const { return m_showYear; }
    QString showImdbId() const { return m_showImdbId; }

private:
    QString m_type;  // 'movie' or 'episode'
    QString m_imdbId;
    QString m_title;
    int m_year;
    int m_season;
    int m_episode;
    QString m_showTitle;
    int m_showYear;
    QString m_showImdbId;
};

/// Trakt Scrobble Response
class TraktScrobbleResponse
{
public:
    TraktScrobbleResponse();
    TraktScrobbleResponse(int id, const QString& action, double progress,
                          const QJsonObject& sharing = QJsonObject(),
                          const TraktMovie& movie = TraktMovie(),
                          const TraktEpisode& episode = TraktEpisode(),
                          const TraktShow& show = TraktShow(),
                          bool alreadyScrobbled = false);
    
    static TraktScrobbleResponse fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    int id() const { return m_id; }
    QString action() const { return m_action; }
    double progress() const { return m_progress; }
    QJsonObject sharing() const { return m_sharing; }
    TraktMovie movie() const { return m_movie; }
    TraktEpisode episode() const { return m_episode; }
    TraktShow show() const { return m_show; }
    bool alreadyScrobbled() const { return m_alreadyScrobbled; }

private:
    int m_id;
    QString m_action;
    double m_progress;
    QJsonObject m_sharing;
    TraktMovie m_movie;
    TraktEpisode m_episode;
    TraktShow m_show;
    bool m_alreadyScrobbled;
};

/// Trakt Playback Item
class TraktPlaybackItem
{
public:
    TraktPlaybackItem();
    TraktPlaybackItem(double progress, const QDateTime& pausedAt, int id, const QString& type,
                     const TraktMovie& movie = TraktMovie(),
                     const TraktEpisode& episode = TraktEpisode(),
                     const TraktShow& show = TraktShow());
    
    static TraktPlaybackItem fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    double progress() const { return m_progress; }
    QDateTime pausedAt() const { return m_pausedAt; }
    int id() const { return m_id; }
    QString type() const { return m_type; }
    TraktMovie movie() const { return m_movie; }
    TraktEpisode episode() const { return m_episode; }
    TraktShow show() const { return m_show; }

private:
    double m_progress;
    QDateTime m_pausedAt;
    int m_id;
    QString m_type;
    TraktMovie m_movie;
    TraktEpisode m_episode;
    TraktShow m_show;
};

/// Trakt Watchlist Item
class TraktWatchlistItem
{
public:
    TraktWatchlistItem();
    TraktWatchlistItem(const QString& type, const TraktMovie& movie = TraktMovie(),
                       const TraktShow& show = TraktShow(), const QDateTime& listedAt = QDateTime(),
                       int rank = 0);
    
    static TraktWatchlistItem fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    QString type() const { return m_type; }
    TraktMovie movie() const { return m_movie; }
    TraktShow show() const { return m_show; }
    QDateTime listedAt() const { return m_listedAt; }
    int rank() const { return m_rank; }

private:
    QString m_type;
    TraktMovie m_movie;
    TraktShow m_show;
    QDateTime m_listedAt;
    int m_rank;
};

/// Trakt Watchlist Item with Images
class TraktWatchlistItemWithImages
{
public:
    TraktWatchlistItemWithImages();
    TraktWatchlistItemWithImages(const QString& type, const TraktMovie& movie = TraktMovie(),
                                 const TraktShow& show = TraktShow(),
                                 const QDateTime& listedAt = QDateTime(),
                                 const TraktImages& images = TraktImages());
    
    static TraktWatchlistItemWithImages fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    QString type() const { return m_type; }
    TraktMovie movie() const { return m_movie; }
    TraktShow show() const { return m_show; }
    QDateTime listedAt() const { return m_listedAt; }
    TraktImages images() const { return m_images; }

private:
    QString m_type;
    TraktMovie m_movie;
    TraktShow m_show;
    QDateTime m_listedAt;
    TraktImages m_images;
};

/// Trakt Collection Item with Images
class TraktCollectionItemWithImages
{
public:
    TraktCollectionItemWithImages();
    TraktCollectionItemWithImages(const QString& type, const TraktMovie& movie = TraktMovie(),
                                  const TraktShow& show = TraktShow(),
                                  const QDateTime& collectedAt = QDateTime(),
                                  const TraktImages& images = TraktImages());
    
    static TraktCollectionItemWithImages fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    QString type() const { return m_type; }
    TraktMovie movie() const { return m_movie; }
    TraktShow show() const { return m_show; }
    QDateTime collectedAt() const { return m_collectedAt; }
    TraktImages images() const { return m_images; }

private:
    QString m_type;
    TraktMovie m_movie;
    TraktShow m_show;
    QDateTime m_collectedAt;
    TraktImages m_images;
};

/// Trakt Rating Item with Images
class TraktRatingItemWithImages
{
public:
    TraktRatingItemWithImages();
    TraktRatingItemWithImages(const QString& type, const TraktMovie& movie = TraktMovie(),
                              const TraktShow& show = TraktShow(), int rating = 0,
                              const QDateTime& ratedAt = QDateTime(),
                              const TraktImages& images = TraktImages());
    
    static TraktRatingItemWithImages fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    QString type() const { return m_type; }
    TraktMovie movie() const { return m_movie; }
    TraktShow show() const { return m_show; }
    int rating() const { return m_rating; }
    QDateTime ratedAt() const { return m_ratedAt; }
    TraktImages images() const { return m_images; }

private:
    QString m_type;
    TraktMovie m_movie;
    TraktShow m_show;
    int m_rating;
    QDateTime m_ratedAt;
    TraktImages m_images;
};

/// Trakt Watched Item
class TraktWatchedItem
{
public:
    TraktWatchedItem();
    TraktWatchedItem(const TraktMovie& movie, const TraktShow& show,
                     int plays, const QDateTime& lastWatchedAt,
                     const QList<QJsonObject>& seasons);
    
    static TraktWatchedItem fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    TraktMovie movie() const { return m_movie; }
    TraktShow show() const { return m_show; }
    int plays() const { return m_plays; }
    QDateTime lastWatchedAt() const { return m_lastWatchedAt; }
    QList<QJsonObject> seasons() const { return m_seasons; }

private:
    TraktMovie m_movie;
    TraktShow m_show;
    int m_plays;
    QDateTime m_lastWatchedAt;
    QList<QJsonObject> m_seasons;
};

/// Trakt History Item
class TraktHistoryItem
{
public:
    TraktHistoryItem();
    TraktHistoryItem(int id, const QDateTime& watchedAt, const QString& action, const QString& type,
                     const TraktMovie& movie = TraktMovie(),
                     const TraktEpisode& episode = TraktEpisode(),
                     const TraktShow& show = TraktShow());
    
    static TraktHistoryItem fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    int id() const { return m_id; }
    QDateTime watchedAt() const { return m_watchedAt; }
    QString action() const { return m_action; }
    QString type() const { return m_type; }
    TraktMovie movie() const { return m_movie; }
    TraktEpisode episode() const { return m_episode; }
    TraktShow show() const { return m_show; }

private:
    int m_id;
    QDateTime m_watchedAt;
    QString m_action;
    QString m_type;
    TraktMovie m_movie;
    TraktEpisode m_episode;
    TraktShow m_show;
};

/// Trakt History Remove Payload
class TraktHistoryRemovePayload
{
public:
    TraktHistoryRemovePayload();
    TraktHistoryRemovePayload(const QList<QJsonObject>& movies = QList<QJsonObject>(),
                              const QList<QJsonObject>& shows = QList<QJsonObject>(),
                              const QList<QJsonObject>& seasons = QList<QJsonObject>(),
                              const QList<QJsonObject>& episodes = QList<QJsonObject>(),
                              const QList<int>& ids = QList<int>());
    
    QJsonObject toJson() const;
    
    QList<QJsonObject> movies() const { return m_movies; }
    QList<QJsonObject> shows() const { return m_shows; }
    QList<QJsonObject> seasons() const { return m_seasons; }
    QList<QJsonObject> episodes() const { return m_episodes; }
    QList<int> ids() const { return m_ids; }

private:
    QList<QJsonObject> m_movies;
    QList<QJsonObject> m_shows;
    QList<QJsonObject> m_seasons;
    QList<QJsonObject> m_episodes;
    QList<int> m_ids;
};

/// Trakt History Remove Response
class TraktHistoryRemoveResponse
{
public:
    TraktHistoryRemoveResponse();
    TraktHistoryRemoveResponse(const QMap<QString, int>& deleted,
                               const QMap<QString, QList<QJsonObject>>& notFound);
    
    static TraktHistoryRemoveResponse fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    QMap<QString, int> deleted() const { return m_deleted; }
    QMap<QString, QList<QJsonObject>> notFound() const { return m_notFound; }

private:
    QMap<QString, int> m_deleted;
    QMap<QString, QList<QJsonObject>> m_notFound;
};

#endif // TRAKT_MODELS_H

