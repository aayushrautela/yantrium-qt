#ifndef TRAKT_MODELS_H
#define TRAKT_MODELS_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QList>
#include <QMap>

// ----------------------------------------------------------------------------
// TraktIds
// ----------------------------------------------------------------------------
struct TraktIds
{
    QString trakt;
    QString slug;
    QString imdb;
    QString tmdb;
    QString tvdb;  // TVDB ID for shows

    static TraktIds fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

// ----------------------------------------------------------------------------
// TraktMovie
// ----------------------------------------------------------------------------
struct TraktMovie
{
    QString title;
    int year = 0;
    TraktIds ids;

    static TraktMovie fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

// ----------------------------------------------------------------------------
// TraktShow
// ----------------------------------------------------------------------------
struct TraktShow
{
    QString title;
    int year = 0;
    TraktIds ids;

    static TraktShow fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

// ----------------------------------------------------------------------------
// TraktEpisode
// ----------------------------------------------------------------------------
struct TraktEpisode
{
    int season = 0;
    int number = 0;
    QString title;
    TraktIds ids;
    int runtime = 0;

    static TraktEpisode fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

// ----------------------------------------------------------------------------
// TraktUser
// ----------------------------------------------------------------------------
struct TraktUser
{
    QString username;
    QString slug;
    QString name;
    bool isPrivate = false;
    bool vip = false;
    bool vipEp = false;
    TraktIds ids;

    static TraktUser fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

// ----------------------------------------------------------------------------
// TraktImages
// ----------------------------------------------------------------------------
struct TraktImages
{
    QStringList fanart;
    QStringList poster;
    QStringList logo;
    QStringList clearart;
    QStringList banner;
    QStringList thumb;

    static TraktImages fromJson(const QJsonObject& json);
    QJsonObject toJson() const;

    // Helper functions
    QString getPosterUrl() const;
    QString getFanartUrl() const;
};

// ----------------------------------------------------------------------------
// TraktContentData
// ----------------------------------------------------------------------------
struct TraktContentData
{
    QString type; // 'movie' or 'episode'
    QString imdbId;
    QString title;
    int year = 0;
    
    // Episode specific
    int season = 0;
    int episode = 0;
    QString showTitle;
    int showYear = 0;
    QString showImdbId;

    QString getContentKey() const;
    QJsonObject toJson() const;
};

// ----------------------------------------------------------------------------
// TraktScrobbleResponse
// ----------------------------------------------------------------------------
struct TraktScrobbleResponse
{
    int id = 0;
    QString action;
    double progress = 0.0;
    bool alreadyScrobbled = false;
    
    QJsonObject sharing;
    TraktMovie movie;
    TraktEpisode episode;
    TraktShow show;

    static TraktScrobbleResponse fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

// ----------------------------------------------------------------------------
// TraktPlaybackItem
// ----------------------------------------------------------------------------
struct TraktPlaybackItem
{
    int id = 0;
    double progress = 0.0;
    QDateTime pausedAt;
    QString type;
    
    TraktMovie movie;
    TraktEpisode episode;
    TraktShow show;

    static TraktPlaybackItem fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

// ----------------------------------------------------------------------------
// TraktWatchlistItem
// ----------------------------------------------------------------------------
struct TraktWatchlistItem
{
    QString type;
    QDateTime listedAt;
    int rank = 0;
    
    TraktMovie movie;
    TraktShow show;

    static TraktWatchlistItem fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

// ----------------------------------------------------------------------------
// TraktWatchlistItemWithImages
// ----------------------------------------------------------------------------
struct TraktWatchlistItemWithImages
{
    QString type;
    QDateTime listedAt;
    
    TraktMovie movie;
    TraktShow show;
    TraktImages images;

    static TraktWatchlistItemWithImages fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

// ----------------------------------------------------------------------------
// TraktCollectionItemWithImages
// ----------------------------------------------------------------------------
struct TraktCollectionItemWithImages
{
    QString type;
    QDateTime collectedAt;
    
    TraktMovie movie;
    TraktShow show;
    TraktImages images;

    static TraktCollectionItemWithImages fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

// ----------------------------------------------------------------------------
// TraktRatingItemWithImages
// ----------------------------------------------------------------------------
struct TraktRatingItemWithImages
{
    QString type;
    int rating = 0;
    QDateTime ratedAt;
    
    TraktMovie movie;
    TraktShow show;
    TraktImages images;

    static TraktRatingItemWithImages fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

// ----------------------------------------------------------------------------
// TraktWatchedItem
// ----------------------------------------------------------------------------
struct TraktWatchedItem
{
    int plays = 0;
    QDateTime lastWatchedAt;
    
    TraktMovie movie;
    TraktShow show;
    QList<QJsonObject> seasons;

    static TraktWatchedItem fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

// ----------------------------------------------------------------------------
// TraktHistoryItem
// ----------------------------------------------------------------------------
struct TraktHistoryItem
{
    int id = 0;
    QDateTime watchedAt;
    QString action;
    QString type;
    
    TraktMovie movie;
    TraktEpisode episode;
    TraktShow show;

    static TraktHistoryItem fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

// ----------------------------------------------------------------------------
// TraktHistoryRemovePayload
// ----------------------------------------------------------------------------
struct TraktHistoryRemovePayload
{
    QList<QJsonObject> movies;
    QList<QJsonObject> shows;
    QList<QJsonObject> seasons;
    QList<QJsonObject> episodes;
    QList<int> ids;

    QJsonObject toJson() const;
};

// ----------------------------------------------------------------------------
// TraktHistoryRemoveResponse
// ----------------------------------------------------------------------------
struct TraktHistoryRemoveResponse
{
    QMap<QString, int> deleted;
    QMap<QString, QList<QJsonObject>> notFound;

    static TraktHistoryRemoveResponse fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

#endif // TRAKT_MODELS_H