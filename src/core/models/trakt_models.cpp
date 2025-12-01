#include "trakt_models.h"
#include <QDebug>
#include <QJsonArray>

// TraktIds
TraktIds::TraktIds()
{
}

TraktIds::TraktIds(const QString& trakt, const QString& slug, const QString& imdb, const QString& tmdb)
    : m_trakt(trakt)
    , m_slug(slug)
    , m_imdb(imdb)
    , m_tmdb(tmdb)
{
}

TraktIds TraktIds::fromJson(const QJsonObject& json)
{
    return TraktIds(
        json["trakt"].toString(),
        json["slug"].toString(),
        json["imdb"].toString(),
        json["tmdb"].toString()
    );
}

QJsonObject TraktIds::toJson() const
{
    QJsonObject obj;
    if (!m_trakt.isEmpty()) {
        obj["trakt"] = m_trakt;
    }
    if (!m_slug.isEmpty()) {
        obj["slug"] = m_slug;
    }
    if (!m_imdb.isEmpty()) {
        obj["imdb"] = m_imdb;
    }
    if (!m_tmdb.isEmpty()) {
        obj["tmdb"] = m_tmdb;
    }
    return obj;
}

// TraktMovie
TraktMovie::TraktMovie()
    : m_year(0)
{
}

TraktMovie::TraktMovie(const QString& title, int year, const TraktIds& ids)
    : m_title(title)
    , m_year(year)
    , m_ids(ids)
{
}

TraktMovie TraktMovie::fromJson(const QJsonObject& json)
{
    return TraktMovie(
        json["title"].toString(""),
        json["year"].toInt(0),
        TraktIds::fromJson(json["ids"].toObject())
    );
}

QJsonObject TraktMovie::toJson() const
{
    QJsonObject obj;
    obj["title"] = m_title;
    if (m_year > 0) {
        obj["year"] = m_year;
    }
    obj["ids"] = m_ids.toJson();
    return obj;
}

// TraktShow
TraktShow::TraktShow()
    : m_year(0)
{
}

TraktShow::TraktShow(const QString& title, int year, const TraktIds& ids)
    : m_title(title)
    , m_year(year)
    , m_ids(ids)
{
}

TraktShow TraktShow::fromJson(const QJsonObject& json)
{
    return TraktShow(
        json["title"].toString(""),
        json["year"].toInt(0),
        TraktIds::fromJson(json["ids"].toObject())
    );
}

QJsonObject TraktShow::toJson() const
{
    QJsonObject obj;
    obj["title"] = m_title;
    if (m_year > 0) {
        obj["year"] = m_year;
    }
    obj["ids"] = m_ids.toJson();
    return obj;
}

// TraktEpisode
TraktEpisode::TraktEpisode()
    : m_season(0)
    , m_number(0)
    , m_runtime(0)
{
}

TraktEpisode::TraktEpisode(int season, int number, const QString& title, const TraktIds& ids, int runtime)
    : m_season(season)
    , m_number(number)
    , m_title(title)
    , m_ids(ids)
    , m_runtime(runtime)
{
}

TraktEpisode TraktEpisode::fromJson(const QJsonObject& json)
{
    return TraktEpisode(
        json["season"].toInt(0),
        json["number"].toInt(0),
        json["title"].toString(""),
        TraktIds::fromJson(json["ids"].toObject()),
        json["runtime"].toInt(0)
    );
}

QJsonObject TraktEpisode::toJson() const
{
    QJsonObject obj;
    obj["season"] = m_season;
    obj["number"] = m_number;
    obj["title"] = m_title;
    obj["ids"] = m_ids.toJson();
    if (m_runtime > 0) {
        obj["runtime"] = m_runtime;
    }
    return obj;
}

// TraktUser
TraktUser::TraktUser()
    : m_private(false)
    , m_vip(false)
    , m_vipEp(false)
{
}

TraktUser::TraktUser(const QString& username, const QString& slug, bool isPrivate, const QString& name,
                     bool vip, bool vipEp, const TraktIds& ids)
    : m_username(username)
    , m_slug(slug)
    , m_private(isPrivate)
    , m_name(name)
    , m_vip(vip)
    , m_vipEp(vipEp)
    , m_ids(ids)
{
}

TraktUser TraktUser::fromJson(const QJsonObject& json)
{
    return TraktUser(
        json["username"].toString(""),
        json["slug"].toString(),
        json["private"].toBool(false),
        json["name"].toString(),
        json["vip"].toBool(false),
        json["vip_ep"].toBool(false),
        TraktIds::fromJson(json["ids"].toObject())
    );
}

QJsonObject TraktUser::toJson() const
{
    QJsonObject obj;
    obj["username"] = m_username;
    if (!m_slug.isEmpty()) {
        obj["slug"] = m_slug;
    }
    obj["private"] = m_private;
    if (!m_name.isEmpty()) {
        obj["name"] = m_name;
    }
    obj["vip"] = m_vip;
    obj["vip_ep"] = m_vipEp;
    obj["ids"] = m_ids.toJson();
    return obj;
}

// TraktImages
TraktImages::TraktImages()
{
}

TraktImages::TraktImages(const QStringList& fanart, const QStringList& poster, const QStringList& logo,
                         const QStringList& clearart, const QStringList& banner, const QStringList& thumb)
    : m_fanart(fanart)
    , m_poster(poster)
    , m_logo(logo)
    , m_clearart(clearart)
    , m_banner(banner)
    , m_thumb(thumb)
{
}

TraktImages TraktImages::fromJson(const QJsonObject& json)
{
    QStringList fanart, poster, logo, clearart, banner, thumb;
    
    if (json.contains("fanart")) {
        QJsonArray arr = json["fanart"].toArray();
        for (const QJsonValue& val : arr) {
            fanart.append(val.toString());
        }
    }
    if (json.contains("poster")) {
        QJsonArray arr = json["poster"].toArray();
        for (const QJsonValue& val : arr) {
            poster.append(val.toString());
        }
    }
    if (json.contains("logo")) {
        QJsonArray arr = json["logo"].toArray();
        for (const QJsonValue& val : arr) {
            logo.append(val.toString());
        }
    }
    if (json.contains("clearart")) {
        QJsonArray arr = json["clearart"].toArray();
        for (const QJsonValue& val : arr) {
            clearart.append(val.toString());
        }
    }
    if (json.contains("banner")) {
        QJsonArray arr = json["banner"].toArray();
        for (const QJsonValue& val : arr) {
            banner.append(val.toString());
        }
    }
    if (json.contains("thumb")) {
        QJsonArray arr = json["thumb"].toArray();
        for (const QJsonValue& val : arr) {
            thumb.append(val.toString());
        }
    }
    
    return TraktImages(fanart, poster, logo, clearart, banner, thumb);
}

QJsonObject TraktImages::toJson() const
{
    QJsonObject obj;
    if (!m_fanart.isEmpty()) {
        obj["fanart"] = QJsonArray::fromStringList(m_fanart);
    }
    if (!m_poster.isEmpty()) {
        obj["poster"] = QJsonArray::fromStringList(m_poster);
    }
    if (!m_logo.isEmpty()) {
        obj["logo"] = QJsonArray::fromStringList(m_logo);
    }
    if (!m_clearart.isEmpty()) {
        obj["clearart"] = QJsonArray::fromStringList(m_clearart);
    }
    if (!m_banner.isEmpty()) {
        obj["banner"] = QJsonArray::fromStringList(m_banner);
    }
    if (!m_thumb.isEmpty()) {
        obj["thumb"] = QJsonArray::fromStringList(m_thumb);
    }
    return obj;
}

QString TraktImages::getPosterUrl() const
{
    if (m_poster.isEmpty()) {
        return QString();
    }
    QString posterPath = m_poster.first();
    return posterPath.startsWith("http") ? posterPath : "https://" + posterPath;
}

QString TraktImages::getFanartUrl() const
{
    if (m_fanart.isEmpty()) {
        return QString();
    }
    QString fanartPath = m_fanart.first();
    return fanartPath.startsWith("http") ? fanartPath : "https://" + fanartPath;
}

// TraktContentData
TraktContentData::TraktContentData()
    : m_year(0)
    , m_season(0)
    , m_episode(0)
    , m_showYear(0)
{
}

TraktContentData::TraktContentData(const QString& type, const QString& imdbId, const QString& title, int year,
                                   int season, int episode, const QString& showTitle,
                                   int showYear, const QString& showImdbId)
    : m_type(type)
    , m_imdbId(imdbId)
    , m_title(title)
    , m_year(year)
    , m_season(season)
    , m_episode(episode)
    , m_showTitle(showTitle)
    , m_showYear(showYear)
    , m_showImdbId(showImdbId)
{
}

QString TraktContentData::getContentKey() const
{
    if (m_type == "movie") {
        return "movie:" + m_imdbId;
    } else {
        QString showId = m_showImdbId.isEmpty() ? m_imdbId : m_showImdbId;
        return QString("episode:%1:S%2E%3").arg(showId).arg(m_season).arg(m_episode);
    }
}

QJsonObject TraktContentData::toJson() const
{
    QJsonObject obj;
    obj["type"] = m_type;
    obj["imdbId"] = m_imdbId;
    obj["title"] = m_title;
    obj["year"] = m_year;
    if (m_season > 0) {
        obj["season"] = m_season;
    }
    if (m_episode > 0) {
        obj["episode"] = m_episode;
    }
    if (!m_showTitle.isEmpty()) {
        obj["showTitle"] = m_showTitle;
    }
    if (m_showYear > 0) {
        obj["showYear"] = m_showYear;
    }
    if (!m_showImdbId.isEmpty()) {
        obj["showImdbId"] = m_showImdbId;
    }
    return obj;
}

// TraktScrobbleResponse
TraktScrobbleResponse::TraktScrobbleResponse()
    : m_id(0)
    , m_action("scrobble")
    , m_progress(0.0)
    , m_alreadyScrobbled(false)
{
}

TraktScrobbleResponse::TraktScrobbleResponse(int id, const QString& action, double progress,
                                            const QJsonObject& sharing,
                                            const TraktMovie& movie,
                                            const TraktEpisode& episode,
                                            const TraktShow& show,
                                            bool alreadyScrobbled)
    : m_id(id)
    , m_action(action)
    , m_progress(progress)
    , m_sharing(sharing)
    , m_movie(movie)
    , m_episode(episode)
    , m_show(show)
    , m_alreadyScrobbled(alreadyScrobbled)
{
}

TraktScrobbleResponse TraktScrobbleResponse::fromJson(const QJsonObject& json)
{
    TraktScrobbleResponse response;
    response.m_id = json["id"].toInt(0);
    response.m_action = json["action"].toString("scrobble");
    response.m_progress = json["progress"].toDouble(0.0);
    response.m_sharing = json["sharing"].toObject();
    response.m_alreadyScrobbled = json["alreadyScrobbled"].toBool(false);
    
    if (json.contains("movie")) {
        response.m_movie = TraktMovie::fromJson(json["movie"].toObject());
    }
    if (json.contains("episode")) {
        response.m_episode = TraktEpisode::fromJson(json["episode"].toObject());
    }
    if (json.contains("show")) {
        response.m_show = TraktShow::fromJson(json["show"].toObject());
    }
    
    return response;
}

QJsonObject TraktScrobbleResponse::toJson() const
{
    QJsonObject obj;
    obj["id"] = m_id;
    obj["action"] = m_action;
    obj["progress"] = m_progress;
    if (!m_sharing.isEmpty()) {
        obj["sharing"] = m_sharing;
    }
    if (!m_movie.title().isEmpty()) {
        obj["movie"] = m_movie.toJson();
    }
    if (m_episode.season() > 0 || m_episode.number() > 0) {
        obj["episode"] = m_episode.toJson();
    }
    if (!m_show.title().isEmpty()) {
        obj["show"] = m_show.toJson();
    }
    if (m_alreadyScrobbled) {
        obj["alreadyScrobbled"] = m_alreadyScrobbled;
    }
    return obj;
}

// TraktPlaybackItem
TraktPlaybackItem::TraktPlaybackItem()
    : m_progress(0.0)
    , m_pausedAt(QDateTime::currentDateTime())
    , m_id(0)
{
}

TraktPlaybackItem::TraktPlaybackItem(double progress, const QDateTime& pausedAt, int id, const QString& type,
                                     const TraktMovie& movie,
                                     const TraktEpisode& episode,
                                     const TraktShow& show)
    : m_progress(progress)
    , m_pausedAt(pausedAt)
    , m_id(id)
    , m_type(type)
    , m_movie(movie)
    , m_episode(episode)
    , m_show(show)
{
}

TraktPlaybackItem TraktPlaybackItem::fromJson(const QJsonObject& json)
{
    TraktPlaybackItem item;
    item.m_progress = json["progress"].toDouble(0.0);
    item.m_pausedAt = QDateTime::fromString(json["paused_at"].toString(), Qt::ISODate);
    item.m_id = json["id"].toInt(0);
    item.m_type = json["type"].toString("");
    
    if (json.contains("movie")) {
        item.m_movie = TraktMovie::fromJson(json["movie"].toObject());
    }
    if (json.contains("episode")) {
        item.m_episode = TraktEpisode::fromJson(json["episode"].toObject());
    }
    if (json.contains("show")) {
        item.m_show = TraktShow::fromJson(json["show"].toObject());
    }
    
    return item;
}

QJsonObject TraktPlaybackItem::toJson() const
{
    QJsonObject obj;
    obj["progress"] = m_progress;
    obj["paused_at"] = m_pausedAt.toString(Qt::ISODate);
    obj["id"] = m_id;
    obj["type"] = m_type;
    if (!m_movie.title().isEmpty()) {
        obj["movie"] = m_movie.toJson();
    }
    if (m_episode.season() > 0 || m_episode.number() > 0) {
        obj["episode"] = m_episode.toJson();
    }
    if (!m_show.title().isEmpty()) {
        obj["show"] = m_show.toJson();
    }
    return obj;
}

// TraktWatchlistItem
TraktWatchlistItem::TraktWatchlistItem()
    : m_listedAt(QDateTime::currentDateTime())
    , m_rank(0)
{
}

TraktWatchlistItem::TraktWatchlistItem(const QString& type, const TraktMovie& movie,
                                       const TraktShow& show, const QDateTime& listedAt,
                                       int rank)
    : m_type(type)
    , m_movie(movie)
    , m_show(show)
    , m_listedAt(listedAt)
    , m_rank(rank)
{
}

TraktWatchlistItem TraktWatchlistItem::fromJson(const QJsonObject& json)
{
    TraktWatchlistItem item;
    item.m_type = json["type"].toString("");
    item.m_listedAt = QDateTime::fromString(json["listed_at"].toString(), Qt::ISODate);
    item.m_rank = json["rank"].toInt(0);
    
    if (json.contains("movie")) {
        item.m_movie = TraktMovie::fromJson(json["movie"].toObject());
    }
    if (json.contains("show")) {
        item.m_show = TraktShow::fromJson(json["show"].toObject());
    }
    
    return item;
}

QJsonObject TraktWatchlistItem::toJson() const
{
    QJsonObject obj;
    obj["type"] = m_type;
    if (!m_movie.title().isEmpty()) {
        obj["movie"] = m_movie.toJson();
    }
    if (!m_show.title().isEmpty()) {
        obj["show"] = m_show.toJson();
    }
    obj["listed_at"] = m_listedAt.toString(Qt::ISODate);
    if (m_rank > 0) {
        obj["rank"] = m_rank;
    }
    return obj;
}

// TraktWatchlistItemWithImages
TraktWatchlistItemWithImages::TraktWatchlistItemWithImages()
    : m_listedAt(QDateTime::currentDateTime())
{
}

TraktWatchlistItemWithImages::TraktWatchlistItemWithImages(const QString& type, const TraktMovie& movie,
                                                           const TraktShow& show,
                                                           const QDateTime& listedAt,
                                                           const TraktImages& images)
    : m_type(type)
    , m_movie(movie)
    , m_show(show)
    , m_listedAt(listedAt)
    , m_images(images)
{
}

TraktWatchlistItemWithImages TraktWatchlistItemWithImages::fromJson(const QJsonObject& json)
{
    TraktWatchlistItemWithImages item;
    item.m_type = json["type"].toString("");
    item.m_listedAt = QDateTime::fromString(json["listed_at"].toString(), Qt::ISODate);
    
    if (json.contains("movie")) {
        item.m_movie = TraktMovie::fromJson(json["movie"].toObject());
    }
    if (json.contains("show")) {
        item.m_show = TraktShow::fromJson(json["show"].toObject());
    }
    if (json.contains("images")) {
        item.m_images = TraktImages::fromJson(json["images"].toObject());
    }
    
    return item;
}

QJsonObject TraktWatchlistItemWithImages::toJson() const
{
    QJsonObject obj;
    obj["type"] = m_type;
    if (!m_movie.title().isEmpty()) {
        obj["movie"] = m_movie.toJson();
    }
    if (!m_show.title().isEmpty()) {
        obj["show"] = m_show.toJson();
    }
    obj["listed_at"] = m_listedAt.toString(Qt::ISODate);
    QJsonObject imagesObj = m_images.toJson();
    if (!imagesObj.isEmpty()) {
        obj["images"] = imagesObj;
    }
    return obj;
}

// TraktCollectionItemWithImages
TraktCollectionItemWithImages::TraktCollectionItemWithImages()
    : m_collectedAt(QDateTime::currentDateTime())
{
}

TraktCollectionItemWithImages::TraktCollectionItemWithImages(const QString& type, const TraktMovie& movie,
                                                             const TraktShow& show,
                                                             const QDateTime& collectedAt,
                                                             const TraktImages& images)
    : m_type(type)
    , m_movie(movie)
    , m_show(show)
    , m_collectedAt(collectedAt)
    , m_images(images)
{
}

TraktCollectionItemWithImages TraktCollectionItemWithImages::fromJson(const QJsonObject& json)
{
    TraktCollectionItemWithImages item;
    item.m_type = json["type"].toString("");
    item.m_collectedAt = QDateTime::fromString(json["collected_at"].toString(), Qt::ISODate);
    
    if (json.contains("movie")) {
        item.m_movie = TraktMovie::fromJson(json["movie"].toObject());
    }
    if (json.contains("show")) {
        item.m_show = TraktShow::fromJson(json["show"].toObject());
    }
    if (json.contains("images")) {
        item.m_images = TraktImages::fromJson(json["images"].toObject());
    }
    
    return item;
}

QJsonObject TraktCollectionItemWithImages::toJson() const
{
    QJsonObject obj;
    obj["type"] = m_type;
    if (!m_movie.title().isEmpty()) {
        obj["movie"] = m_movie.toJson();
    }
    if (!m_show.title().isEmpty()) {
        obj["show"] = m_show.toJson();
    }
    obj["collected_at"] = m_collectedAt.toString(Qt::ISODate);
    QJsonObject imagesObj = m_images.toJson();
    if (!imagesObj.isEmpty()) {
        obj["images"] = imagesObj;
    }
    return obj;
}

// TraktRatingItemWithImages
TraktRatingItemWithImages::TraktRatingItemWithImages()
    : m_rating(0)
    , m_ratedAt(QDateTime::currentDateTime())
{
}

TraktRatingItemWithImages::TraktRatingItemWithImages(const QString& type, const TraktMovie& movie,
                                                     const TraktShow& show, int rating,
                                                     const QDateTime& ratedAt,
                                                     const TraktImages& images)
    : m_type(type)
    , m_movie(movie)
    , m_show(show)
    , m_rating(rating)
    , m_ratedAt(ratedAt)
    , m_images(images)
{
}

TraktRatingItemWithImages TraktRatingItemWithImages::fromJson(const QJsonObject& json)
{
    TraktRatingItemWithImages item;
    item.m_type = json["type"].toString("");
    item.m_rating = json["rating"].toInt(0);
    item.m_ratedAt = QDateTime::fromString(json["rated_at"].toString(), Qt::ISODate);
    
    if (json.contains("movie")) {
        item.m_movie = TraktMovie::fromJson(json["movie"].toObject());
    }
    if (json.contains("show")) {
        item.m_show = TraktShow::fromJson(json["show"].toObject());
    }
    if (json.contains("images")) {
        item.m_images = TraktImages::fromJson(json["images"].toObject());
    }
    
    return item;
}

QJsonObject TraktRatingItemWithImages::toJson() const
{
    QJsonObject obj;
    obj["type"] = m_type;
    if (!m_movie.title().isEmpty()) {
        obj["movie"] = m_movie.toJson();
    }
    if (!m_show.title().isEmpty()) {
        obj["show"] = m_show.toJson();
    }
    obj["rating"] = m_rating;
    obj["rated_at"] = m_ratedAt.toString(Qt::ISODate);
    QJsonObject imagesObj = m_images.toJson();
    if (!imagesObj.isEmpty()) {
        obj["images"] = imagesObj;
    }
    return obj;
}

// TraktWatchedItem
TraktWatchedItem::TraktWatchedItem()
    : m_plays(0)
    , m_lastWatchedAt(QDateTime::currentDateTime())
{
}

TraktWatchedItem::TraktWatchedItem(const TraktMovie& movie, const TraktShow& show,
                                  int plays, const QDateTime& lastWatchedAt,
                                  const QList<QJsonObject>& seasons)
    : m_movie(movie)
    , m_show(show)
    , m_plays(plays)
    , m_lastWatchedAt(lastWatchedAt)
    , m_seasons(seasons)
{
}

TraktWatchedItem TraktWatchedItem::fromJson(const QJsonObject& json)
{
    TraktWatchedItem item;
    item.m_plays = json["plays"].toInt(0);
    item.m_lastWatchedAt = QDateTime::fromString(json["last_watched_at"].toString(), Qt::ISODate);
    
    if (json.contains("movie")) {
        item.m_movie = TraktMovie::fromJson(json["movie"].toObject());
    }
    if (json.contains("show")) {
        item.m_show = TraktShow::fromJson(json["show"].toObject());
    }
    if (json.contains("seasons")) {
        QJsonArray seasonsArr = json["seasons"].toArray();
        for (const QJsonValue& val : seasonsArr) {
            item.m_seasons.append(val.toObject());
        }
    }
    
    return item;
}

QJsonObject TraktWatchedItem::toJson() const
{
    QJsonObject obj;
    if (!m_movie.title().isEmpty()) {
        obj["movie"] = m_movie.toJson();
    }
    if (!m_show.title().isEmpty()) {
        obj["show"] = m_show.toJson();
    }
    obj["plays"] = m_plays;
    obj["last_watched_at"] = m_lastWatchedAt.toString(Qt::ISODate);
    if (!m_seasons.isEmpty()) {
        QJsonArray seasonsArr;
        for (const QJsonObject& season : m_seasons) {
            seasonsArr.append(season);
        }
        obj["seasons"] = seasonsArr;
    }
    return obj;
}

// TraktHistoryItem
TraktHistoryItem::TraktHistoryItem()
    : m_id(0)
    , m_watchedAt(QDateTime::currentDateTime())
{
}

TraktHistoryItem::TraktHistoryItem(int id, const QDateTime& watchedAt, const QString& action, const QString& type,
                                  const TraktMovie& movie,
                                  const TraktEpisode& episode,
                                  const TraktShow& show)
    : m_id(id)
    , m_watchedAt(watchedAt)
    , m_action(action)
    , m_type(type)
    , m_movie(movie)
    , m_episode(episode)
    , m_show(show)
{
}

TraktHistoryItem TraktHistoryItem::fromJson(const QJsonObject& json)
{
    TraktHistoryItem item;
    item.m_id = json["id"].toInt(0);
    item.m_watchedAt = QDateTime::fromString(json["watched_at"].toString(), Qt::ISODate);
    item.m_action = json["action"].toString("watch");
    item.m_type = json["type"].toString("");
    
    if (json.contains("movie")) {
        item.m_movie = TraktMovie::fromJson(json["movie"].toObject());
    }
    if (json.contains("episode")) {
        item.m_episode = TraktEpisode::fromJson(json["episode"].toObject());
    }
    if (json.contains("show")) {
        item.m_show = TraktShow::fromJson(json["show"].toObject());
    }
    
    return item;
}

QJsonObject TraktHistoryItem::toJson() const
{
    QJsonObject obj;
    obj["id"] = m_id;
    obj["watched_at"] = m_watchedAt.toString(Qt::ISODate);
    obj["action"] = m_action;
    obj["type"] = m_type;
    if (!m_movie.title().isEmpty()) {
        obj["movie"] = m_movie.toJson();
    }
    if (m_episode.season() > 0 || m_episode.number() > 0) {
        obj["episode"] = m_episode.toJson();
    }
    if (!m_show.title().isEmpty()) {
        obj["show"] = m_show.toJson();
    }
    return obj;
}

// TraktHistoryRemovePayload
TraktHistoryRemovePayload::TraktHistoryRemovePayload()
{
}

TraktHistoryRemovePayload::TraktHistoryRemovePayload(const QList<QJsonObject>& movies,
                                                    const QList<QJsonObject>& shows,
                                                    const QList<QJsonObject>& seasons,
                                                    const QList<QJsonObject>& episodes,
                                                    const QList<int>& ids)
    : m_movies(movies)
    , m_shows(shows)
    , m_seasons(seasons)
    , m_episodes(episodes)
    , m_ids(ids)
{
}

QJsonObject TraktHistoryRemovePayload::toJson() const
{
    QJsonObject obj;
    if (!m_movies.isEmpty()) {
        QJsonArray arr;
        for (const QJsonObject& movie : m_movies) {
            arr.append(movie);
        }
        obj["movies"] = arr;
    }
    if (!m_shows.isEmpty()) {
        QJsonArray arr;
        for (const QJsonObject& show : m_shows) {
            arr.append(show);
        }
        obj["shows"] = arr;
    }
    if (!m_seasons.isEmpty()) {
        QJsonArray arr;
        for (const QJsonObject& season : m_seasons) {
            arr.append(season);
        }
        obj["seasons"] = arr;
    }
    if (!m_episodes.isEmpty()) {
        QJsonArray arr;
        for (const QJsonObject& episode : m_episodes) {
            arr.append(episode);
        }
        obj["episodes"] = arr;
    }
    if (!m_ids.isEmpty()) {
        QJsonArray arr;
        for (int id : m_ids) {
            arr.append(id);
        }
        obj["ids"] = arr;
    }
    return obj;
}

// TraktHistoryRemoveResponse
TraktHistoryRemoveResponse::TraktHistoryRemoveResponse()
{
}

TraktHistoryRemoveResponse::TraktHistoryRemoveResponse(const QMap<QString, int>& deleted,
                                                       const QMap<QString, QList<QJsonObject>>& notFound)
    : m_deleted(deleted)
    , m_notFound(notFound)
{
}

TraktHistoryRemoveResponse TraktHistoryRemoveResponse::fromJson(const QJsonObject& json)
{
    TraktHistoryRemoveResponse response;
    
    QJsonObject deletedObj = json["deleted"].toObject();
    for (auto it = deletedObj.begin(); it != deletedObj.end(); ++it) {
        response.m_deleted[it.key()] = it.value().toInt();
    }
    
    QJsonObject notFoundObj = json["not_found"].toObject();
    for (auto it = notFoundObj.begin(); it != notFoundObj.end(); ++it) {
        QJsonArray arr = it.value().toArray();
        QList<QJsonObject> list;
        for (const QJsonValue& val : arr) {
            list.append(val.toObject());
        }
        response.m_notFound[it.key()] = list;
    }
    
    return response;
}

QJsonObject TraktHistoryRemoveResponse::toJson() const
{
    QJsonObject obj;
    
    QJsonObject deletedObj;
    for (auto it = m_deleted.begin(); it != m_deleted.end(); ++it) {
        deletedObj[it.key()] = it.value();
    }
    obj["deleted"] = deletedObj;
    
    QJsonObject notFoundObj;
    for (auto it = m_notFound.begin(); it != m_notFound.end(); ++it) {
        QJsonArray arr;
        for (const QJsonObject& item : it.value()) {
            arr.append(item);
        }
        notFoundObj[it.key()] = arr;
    }
    obj["not_found"] = notFoundObj;
    
    return obj;
}



