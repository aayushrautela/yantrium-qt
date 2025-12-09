#include "trakt_models.h"

// ----------------------------------------------------------------------------
// TraktIds
// ----------------------------------------------------------------------------
TraktIds TraktIds::fromJson(const QJsonObject& json)
{
    TraktIds obj;
    obj.trakt = json["trakt"].toString();
    obj.slug = json["slug"].toString();
    obj.imdb = json["imdb"].toString();
    obj.tmdb = json["tmdb"].toString();
    obj.tvdb = json["tvdb"].toString();  // TVDB ID (for shows)
    return obj;
}

QJsonObject TraktIds::toJson() const
{
    QJsonObject obj;
    if (!trakt.isEmpty()) obj["trakt"] = trakt;
    if (!slug.isEmpty())  obj["slug"] = slug;
    if (!imdb.isEmpty())  obj["imdb"] = imdb;
    if (!tmdb.isEmpty())  obj["tmdb"] = tmdb;
    if (!tvdb.isEmpty())  obj["tvdb"] = tvdb;
    return obj;
}

// ----------------------------------------------------------------------------
// TraktMovie
// ----------------------------------------------------------------------------
TraktMovie TraktMovie::fromJson(const QJsonObject& json)
{
    TraktMovie obj;
    obj.title = json["title"].toString();
    obj.year = json["year"].toInt(0);
    obj.ids = TraktIds::fromJson(json["ids"].toObject());
    return obj;
}

QJsonObject TraktMovie::toJson() const
{
    QJsonObject obj;
    obj["title"] = title;
    if (year > 0) obj["year"] = year;
    obj["ids"] = ids.toJson();
    return obj;
}

// ----------------------------------------------------------------------------
// TraktShow
// ----------------------------------------------------------------------------
TraktShow TraktShow::fromJson(const QJsonObject& json)
{
    TraktShow obj;
    obj.title = json["title"].toString();
    obj.year = json["year"].toInt(0);
    obj.ids = TraktIds::fromJson(json["ids"].toObject());
    return obj;
}

QJsonObject TraktShow::toJson() const
{
    QJsonObject obj;
    obj["title"] = title;
    if (year > 0) obj["year"] = year;
    obj["ids"] = ids.toJson();
    return obj;
}

// ----------------------------------------------------------------------------
// TraktEpisode
// ----------------------------------------------------------------------------
TraktEpisode TraktEpisode::fromJson(const QJsonObject& json)
{
    TraktEpisode obj;
    obj.season = json["season"].toInt(0);
    obj.number = json["number"].toInt(0);
    obj.title = json["title"].toString();
    obj.ids = TraktIds::fromJson(json["ids"].toObject());
    obj.runtime = json["runtime"].toInt(0);
    return obj;
}

QJsonObject TraktEpisode::toJson() const
{
    QJsonObject obj;
    obj["season"] = season;
    obj["number"] = number;
    obj["title"] = title;
    obj["ids"] = ids.toJson();
    if (runtime > 0) obj["runtime"] = runtime;
    return obj;
}

// ----------------------------------------------------------------------------
// TraktUser
// ----------------------------------------------------------------------------
TraktUser TraktUser::fromJson(const QJsonObject& json)
{
    TraktUser obj;
    obj.username = json["username"].toString();
    obj.slug = json["slug"].toString();
    obj.isPrivate = json["private"].toBool(false);
    obj.name = json["name"].toString();
    obj.vip = json["vip"].toBool(false);
    obj.vipEp = json["vip_ep"].toBool(false);
    obj.ids = TraktIds::fromJson(json["ids"].toObject());
    return obj;
}

QJsonObject TraktUser::toJson() const
{
    QJsonObject obj;
    obj["username"] = username;
    if (!slug.isEmpty()) obj["slug"] = slug;
    obj["private"] = isPrivate;
    if (!name.isEmpty()) obj["name"] = name;
    obj["vip"] = vip;
    obj["vip_ep"] = vipEp;
    obj["ids"] = ids.toJson();
    return obj;
}

// ----------------------------------------------------------------------------
// TraktImages
// ----------------------------------------------------------------------------
TraktImages TraktImages::fromJson(const QJsonObject& json)
{
    TraktImages obj;

    auto parseList = [](const QJsonObject& j, const QString& key) -> QStringList {
        QStringList list;
        if (j.contains(key)) {
            QJsonArray arr = j[key].toArray();
            for (const QJsonValue& val : arr) list.append(val.toString());
        }
        return list;
    };

    obj.fanart = parseList(json, "fanart");
    obj.poster = parseList(json, "poster");
    obj.logo = parseList(json, "logo");
    obj.clearart = parseList(json, "clearart");
    obj.banner = parseList(json, "banner");
    obj.thumb = parseList(json, "thumb");

    return obj;
}

QJsonObject TraktImages::toJson() const
{
    QJsonObject obj;
    if (!fanart.isEmpty())   obj["fanart"] = QJsonArray::fromStringList(fanart);
    if (!poster.isEmpty())   obj["poster"] = QJsonArray::fromStringList(poster);
    if (!logo.isEmpty())     obj["logo"] = QJsonArray::fromStringList(logo);
    if (!clearart.isEmpty()) obj["clearart"] = QJsonArray::fromStringList(clearart);
    if (!banner.isEmpty())   obj["banner"] = QJsonArray::fromStringList(banner);
    if (!thumb.isEmpty())    obj["thumb"] = QJsonArray::fromStringList(thumb);
    return obj;
}

QString TraktImages::getPosterUrl() const
{
    if (poster.isEmpty()) return QString();
    QString path = poster.first();
    return path.startsWith("http") ? path : "https://" + path;
}

QString TraktImages::getFanartUrl() const
{
    if (fanart.isEmpty()) return QString();
    QString path = fanart.first();
    return path.startsWith("http") ? path : "https://" + path;
}

// ----------------------------------------------------------------------------
// TraktContentData
// ----------------------------------------------------------------------------
QString TraktContentData::getContentKey() const
{
    if (type == "movie") {
        return "movie:" + imdbId;
    } else {
        QString showId = showImdbId.isEmpty() ? imdbId : showImdbId;
        return QString("episode:%1:S%2E%3").arg(showId).arg(season).arg(episode);
    }
}

QJsonObject TraktContentData::toJson() const
{
    QJsonObject obj;
    obj["type"] = type;
    obj["imdbId"] = imdbId;
    obj["title"] = title;
    obj["year"] = year;
    if (season > 0) obj["season"] = season;
    if (episode > 0) obj["episode"] = episode;
    if (!showTitle.isEmpty()) obj["showTitle"] = showTitle;
    if (showYear > 0) obj["showYear"] = showYear;
    if (!showImdbId.isEmpty()) obj["showImdbId"] = showImdbId;
    return obj;
}

// ----------------------------------------------------------------------------
// TraktScrobbleResponse
// ----------------------------------------------------------------------------
TraktScrobbleResponse TraktScrobbleResponse::fromJson(const QJsonObject& json)
{
    TraktScrobbleResponse obj;
    obj.id = json["id"].toInt(0);
    obj.action = json["action"].toString("scrobble");
    obj.progress = json["progress"].toDouble(0.0);
    obj.sharing = json["sharing"].toObject();
    obj.alreadyScrobbled = json["alreadyScrobbled"].toBool(false);
    
    if (json.contains("movie"))   obj.movie = TraktMovie::fromJson(json["movie"].toObject());
    if (json.contains("episode")) obj.episode = TraktEpisode::fromJson(json["episode"].toObject());
    if (json.contains("show"))    obj.show = TraktShow::fromJson(json["show"].toObject());
    
    return obj;
}

QJsonObject TraktScrobbleResponse::toJson() const
{
    QJsonObject obj;
    obj["id"] = id;
    obj["action"] = action;
    obj["progress"] = progress;
    if (!sharing.isEmpty()) obj["sharing"] = sharing;
    if (!movie.title.isEmpty()) obj["movie"] = movie.toJson();
    if (episode.season > 0 || episode.number > 0) obj["episode"] = episode.toJson();
    if (!show.title.isEmpty()) obj["show"] = show.toJson();
    if (alreadyScrobbled) obj["alreadyScrobbled"] = alreadyScrobbled;
    return obj;
}

// ----------------------------------------------------------------------------
// TraktPlaybackItem
// ----------------------------------------------------------------------------
TraktPlaybackItem TraktPlaybackItem::fromJson(const QJsonObject& json)
{
    TraktPlaybackItem obj;
    obj.progress = json["progress"].toDouble(0.0);
    obj.pausedAt = QDateTime::fromString(json["paused_at"].toString(), Qt::ISODate);
    obj.id = json["id"].toInt(0);
    obj.type = json["type"].toString();

    if (json.contains("movie"))   obj.movie = TraktMovie::fromJson(json["movie"].toObject());
    if (json.contains("episode")) obj.episode = TraktEpisode::fromJson(json["episode"].toObject());
    if (json.contains("show"))    obj.show = TraktShow::fromJson(json["show"].toObject());

    return obj;
}

QJsonObject TraktPlaybackItem::toJson() const
{
    QJsonObject obj;
    obj["progress"] = progress;
    obj["paused_at"] = pausedAt.toString(Qt::ISODate);
    obj["id"] = id;
    obj["type"] = type;
    if (!movie.title.isEmpty()) obj["movie"] = movie.toJson();
    if (episode.season > 0 || episode.number > 0) obj["episode"] = episode.toJson();
    if (!show.title.isEmpty()) obj["show"] = show.toJson();
    return obj;
}

// ----------------------------------------------------------------------------
// TraktWatchlistItem
// ----------------------------------------------------------------------------
TraktWatchlistItem TraktWatchlistItem::fromJson(const QJsonObject& json)
{
    TraktWatchlistItem obj;
    obj.type = json["type"].toString();
    obj.listedAt = QDateTime::fromString(json["listed_at"].toString(), Qt::ISODate);
    obj.rank = json["rank"].toInt(0);

    if (json.contains("movie")) obj.movie = TraktMovie::fromJson(json["movie"].toObject());
    if (json.contains("show"))  obj.show = TraktShow::fromJson(json["show"].toObject());

    return obj;
}

QJsonObject TraktWatchlistItem::toJson() const
{
    QJsonObject obj;
    obj["type"] = type;
    if (!movie.title.isEmpty()) obj["movie"] = movie.toJson();
    if (!show.title.isEmpty())  obj["show"] = show.toJson();
    obj["listed_at"] = listedAt.toString(Qt::ISODate);
    if (rank > 0) obj["rank"] = rank;
    return obj;
}

// ----------------------------------------------------------------------------
// TraktWatchlistItemWithImages
// ----------------------------------------------------------------------------
TraktWatchlistItemWithImages TraktWatchlistItemWithImages::fromJson(const QJsonObject& json)
{
    TraktWatchlistItemWithImages obj;
    obj.type = json["type"].toString();
    obj.listedAt = QDateTime::fromString(json["listed_at"].toString(), Qt::ISODate);

    if (json.contains("movie"))  obj.movie = TraktMovie::fromJson(json["movie"].toObject());
    if (json.contains("show"))   obj.show = TraktShow::fromJson(json["show"].toObject());
    if (json.contains("images")) obj.images = TraktImages::fromJson(json["images"].toObject());

    return obj;
}

QJsonObject TraktWatchlistItemWithImages::toJson() const
{
    QJsonObject obj;
    obj["type"] = type;
    if (!movie.title.isEmpty()) obj["movie"] = movie.toJson();
    if (!show.title.isEmpty())  obj["show"] = show.toJson();
    obj["listed_at"] = listedAt.toString(Qt::ISODate);
    
    QJsonObject imgObj = images.toJson();
    if (!imgObj.isEmpty()) obj["images"] = imgObj;
    
    return obj;
}

// ----------------------------------------------------------------------------
// TraktCollectionItemWithImages
// ----------------------------------------------------------------------------
TraktCollectionItemWithImages TraktCollectionItemWithImages::fromJson(const QJsonObject& json)
{
    TraktCollectionItemWithImages obj;
    obj.type = json["type"].toString();
    obj.collectedAt = QDateTime::fromString(json["collected_at"].toString(), Qt::ISODate);

    if (json.contains("movie"))  obj.movie = TraktMovie::fromJson(json["movie"].toObject());
    if (json.contains("show"))   obj.show = TraktShow::fromJson(json["show"].toObject());
    if (json.contains("images")) obj.images = TraktImages::fromJson(json["images"].toObject());

    return obj;
}

QJsonObject TraktCollectionItemWithImages::toJson() const
{
    QJsonObject obj;
    obj["type"] = type;
    if (!movie.title.isEmpty()) obj["movie"] = movie.toJson();
    if (!show.title.isEmpty())  obj["show"] = show.toJson();
    obj["collected_at"] = collectedAt.toString(Qt::ISODate);

    QJsonObject imgObj = images.toJson();
    if (!imgObj.isEmpty()) obj["images"] = imgObj;
    
    return obj;
}

// ----------------------------------------------------------------------------
// TraktRatingItemWithImages
// ----------------------------------------------------------------------------
TraktRatingItemWithImages TraktRatingItemWithImages::fromJson(const QJsonObject& json)
{
    TraktRatingItemWithImages obj;
    obj.type = json["type"].toString();
    obj.rating = json["rating"].toInt(0);
    obj.ratedAt = QDateTime::fromString(json["rated_at"].toString(), Qt::ISODate);

    if (json.contains("movie"))  obj.movie = TraktMovie::fromJson(json["movie"].toObject());
    if (json.contains("show"))   obj.show = TraktShow::fromJson(json["show"].toObject());
    if (json.contains("images")) obj.images = TraktImages::fromJson(json["images"].toObject());

    return obj;
}

QJsonObject TraktRatingItemWithImages::toJson() const
{
    QJsonObject obj;
    obj["type"] = type;
    if (!movie.title.isEmpty()) obj["movie"] = movie.toJson();
    if (!show.title.isEmpty())  obj["show"] = show.toJson();
    obj["rating"] = rating;
    obj["rated_at"] = ratedAt.toString(Qt::ISODate);

    QJsonObject imgObj = images.toJson();
    if (!imgObj.isEmpty()) obj["images"] = imgObj;
    
    return obj;
}

// ----------------------------------------------------------------------------
// TraktWatchedItem
// ----------------------------------------------------------------------------
TraktWatchedItem TraktWatchedItem::fromJson(const QJsonObject& json)
{
    TraktWatchedItem obj;
    obj.plays = json["plays"].toInt(0);
    obj.lastWatchedAt = QDateTime::fromString(json["last_watched_at"].toString(), Qt::ISODate);

    if (json.contains("movie")) obj.movie = TraktMovie::fromJson(json["movie"].toObject());
    if (json.contains("show"))  obj.show = TraktShow::fromJson(json["show"].toObject());
    
    if (json.contains("seasons")) {
        QJsonArray arr = json["seasons"].toArray();
        for (const QJsonValue& val : arr) {
            obj.seasons.append(val.toObject());
        }
    }
    return obj;
}

QJsonObject TraktWatchedItem::toJson() const
{
    QJsonObject obj;
    if (!movie.title.isEmpty()) obj["movie"] = movie.toJson();
    if (!show.title.isEmpty())  obj["show"] = show.toJson();
    obj["plays"] = plays;
    obj["last_watched_at"] = lastWatchedAt.toString(Qt::ISODate);

    if (!seasons.isEmpty()) {
        QJsonArray arr;
        for (const QJsonObject& s : seasons) arr.append(s);
        obj["seasons"] = arr;
    }
    return obj;
}

// ----------------------------------------------------------------------------
// TraktHistoryItem
// ----------------------------------------------------------------------------
TraktHistoryItem TraktHistoryItem::fromJson(const QJsonObject& json)
{
    TraktHistoryItem obj;
    obj.id = json["id"].toInt(0);
    obj.watchedAt = QDateTime::fromString(json["watched_at"].toString(), Qt::ISODate);
    obj.action = json["action"].toString("watch");
    obj.type = json["type"].toString();

    if (json.contains("movie"))   obj.movie = TraktMovie::fromJson(json["movie"].toObject());
    if (json.contains("episode")) obj.episode = TraktEpisode::fromJson(json["episode"].toObject());
    if (json.contains("show"))    obj.show = TraktShow::fromJson(json["show"].toObject());

    return obj;
}

QJsonObject TraktHistoryItem::toJson() const
{
    QJsonObject obj;
    obj["id"] = id;
    obj["watched_at"] = watchedAt.toString(Qt::ISODate);
    obj["action"] = action;
    obj["type"] = type;

    if (!movie.title.isEmpty()) obj["movie"] = movie.toJson();
    if (episode.season > 0 || episode.number > 0) obj["episode"] = episode.toJson();
    if (!show.title.isEmpty()) obj["show"] = show.toJson();
    return obj;
}

// ----------------------------------------------------------------------------
// TraktHistoryRemovePayload
// ----------------------------------------------------------------------------
QJsonObject TraktHistoryRemovePayload::toJson() const
{
    QJsonObject obj;
    
    if (!movies.isEmpty()) {
        QJsonArray arr;
        for (const QJsonObject& i : movies) arr.append(i);
        obj["movies"] = arr;
    }
    if (!shows.isEmpty()) {
        QJsonArray arr;
        for (const QJsonObject& i : shows) arr.append(i);
        obj["shows"] = arr;
    }
    if (!seasons.isEmpty()) {
        QJsonArray arr;
        for (const QJsonObject& i : seasons) arr.append(i);
        obj["seasons"] = arr;
    }
    if (!episodes.isEmpty()) {
        QJsonArray arr;
        for (const QJsonObject& i : episodes) arr.append(i);
        obj["episodes"] = arr;
    }
    if (!ids.isEmpty()) {
        QJsonArray arr;
        for (int id : ids) arr.append(id);
        obj["ids"] = arr;
    }
    return obj;
}

// ----------------------------------------------------------------------------
// TraktHistoryRemoveResponse
// ----------------------------------------------------------------------------
TraktHistoryRemoveResponse TraktHistoryRemoveResponse::fromJson(const QJsonObject& json)
{
    TraktHistoryRemoveResponse obj;

    QJsonObject delObj = json["deleted"].toObject();
    for (auto it = delObj.begin(); it != delObj.end(); ++it) {
        obj.deleted[it.key()] = it.value().toInt();
    }

    QJsonObject nfObj = json["not_found"].toObject();
    for (auto it = nfObj.begin(); it != nfObj.end(); ++it) {
        QJsonArray arr = it.value().toArray();
        QList<QJsonObject> list;
        for (const QJsonValue& val : arr) list.append(val.toObject());
        obj.notFound[it.key()] = list;
    }

    return obj;
}

QJsonObject TraktHistoryRemoveResponse::toJson() const
{
    QJsonObject obj;

    QJsonObject delObj;
    for (auto it = deleted.begin(); it != deleted.end(); ++it) {
        delObj[it.key()] = it.value();
    }
    obj["deleted"] = delObj;

    QJsonObject nfObj;
    for (auto it = notFound.begin(); it != notFound.end(); ++it) {
        QJsonArray arr;
        for (const QJsonObject& val : it.value()) arr.append(val);
        nfObj[it.key()] = arr;
    }
    obj["not_found"] = nfObj;

    return obj;
}