#include "tmdb_models.h"
#include <QDebug>

// TmdbIds
TmdbIds::TmdbIds()
    : m_tmdb(0)
{
}

TmdbIds::TmdbIds(const QString& imdb, int tmdb)
    : m_imdb(imdb)
    , m_tmdb(tmdb)
{
}

TmdbIds TmdbIds::fromJson(const QJsonObject& json)
{
    return TmdbIds(
        json["imdb_id"].toString(),
        json["id"].toInt()
    );
}

QJsonObject TmdbIds::toJson() const
{
    QJsonObject obj;
    if (!m_imdb.isEmpty()) {
        obj["imdb_id"] = m_imdb;
    }
    if (m_tmdb > 0) {
        obj["id"] = m_tmdb;
    }
    return obj;
}

// TmdbGenre
TmdbGenre::TmdbGenre()
    : m_id(0)
{
}

TmdbGenre::TmdbGenre(int id, const QString& name)
    : m_id(id)
    , m_name(name)
{
}

TmdbGenre TmdbGenre::fromJson(const QJsonObject& json)
{
    return TmdbGenre(
        json["id"].toInt(0),
        json["name"].toString("")
    );
}

QJsonObject TmdbGenre::toJson() const
{
    QJsonObject obj;
    obj["id"] = m_id;
    obj["name"] = m_name;
    return obj;
}

// TmdbProductionCompany
TmdbProductionCompany::TmdbProductionCompany()
    : m_id(0)
{
}

TmdbProductionCompany::TmdbProductionCompany(int id, const QString& name, const QString& logoPath, const QString& originCountry)
    : m_id(id)
    , m_name(name)
    , m_logoPath(logoPath)
    , m_originCountry(originCountry)
{
}

TmdbProductionCompany TmdbProductionCompany::fromJson(const QJsonObject& json)
{
    return TmdbProductionCompany(
        json["id"].toInt(0),
        json["name"].toString(""),
        json["logo_path"].toString(),
        json["origin_country"].toString()
    );
}

QJsonObject TmdbProductionCompany::toJson() const
{
    QJsonObject obj;
    obj["id"] = m_id;
    obj["name"] = m_name;
    if (!m_logoPath.isEmpty()) {
        obj["logo_path"] = m_logoPath;
    }
    if (!m_originCountry.isEmpty()) {
        obj["origin_country"] = m_originCountry;
    }
    return obj;
}

// TmdbSpokenLanguage
TmdbSpokenLanguage::TmdbSpokenLanguage()
{
}

TmdbSpokenLanguage::TmdbSpokenLanguage(const QString& iso6391, const QString& name)
    : m_iso6391(iso6391)
    , m_name(name)
{
}

TmdbSpokenLanguage TmdbSpokenLanguage::fromJson(const QJsonObject& json)
{
    return TmdbSpokenLanguage(
        json["iso_639_1"].toString(""),
        json["name"].toString("")
    );
}

QJsonObject TmdbSpokenLanguage::toJson() const
{
    QJsonObject obj;
    obj["iso_639_1"] = m_iso6391;
    obj["name"] = m_name;
    return obj;
}

// TmdbSearchResult
TmdbSearchResult::TmdbSearchResult()
    : m_id(0)
    , m_voteAverage(0.0)
    , m_voteCount(0)
    , m_popularity(0.0)
    , m_adult(false)
{
}

TmdbSearchResult::TmdbSearchResult(int id, const QString& title, const QString& name, const QString& overview,
                                   const QString& releaseDate, const QString& firstAirDate,
                                   const QString& posterPath, const QString& backdropPath,
                                   double voteAverage, int voteCount, double popularity, bool adult,
                                   const QString& mediaType)
    : m_id(id)
    , m_title(title)
    , m_name(name)
    , m_overview(overview)
    , m_releaseDate(releaseDate)
    , m_firstAirDate(firstAirDate)
    , m_posterPath(posterPath)
    , m_backdropPath(backdropPath)
    , m_voteAverage(voteAverage)
    , m_voteCount(voteCount)
    , m_popularity(popularity)
    , m_adult(adult)
    , m_mediaType(mediaType)
{
}

TmdbSearchResult TmdbSearchResult::fromJson(const QJsonObject& json)
{
    return TmdbSearchResult(
        json["id"].toInt(0),
        json["title"].toString(),
        json["name"].toString(),
        json["overview"].toString(),
        json["release_date"].toString(),
        json["first_air_date"].toString(),
        json["poster_path"].toString(),
        json["backdrop_path"].toString(),
        json["vote_average"].toDouble(0.0),
        json["vote_count"].toInt(0),
        json["popularity"].toDouble(0.0),
        json["adult"].toBool(false),
        json["media_type"].toString()
    );
}

QJsonObject TmdbSearchResult::toJson() const
{
    QJsonObject obj;
    obj["id"] = m_id;
    if (!m_title.isEmpty()) obj["title"] = m_title;
    if (!m_name.isEmpty()) obj["name"] = m_name;
    if (!m_overview.isEmpty()) obj["overview"] = m_overview;
    if (!m_releaseDate.isEmpty()) obj["release_date"] = m_releaseDate;
    if (!m_firstAirDate.isEmpty()) obj["first_air_date"] = m_firstAirDate;
    if (!m_posterPath.isEmpty()) obj["poster_path"] = m_posterPath;
    if (!m_backdropPath.isEmpty()) obj["backdrop_path"] = m_backdropPath;
    obj["vote_average"] = m_voteAverage;
    obj["vote_count"] = m_voteCount;
    obj["popularity"] = m_popularity;
    obj["adult"] = m_adult;
    if (!m_mediaType.isEmpty()) obj["media_type"] = m_mediaType;
    return obj;
}

// TmdbFindResult
TmdbFindResult::TmdbFindResult()
{
}

TmdbFindResult::TmdbFindResult(const QList<TmdbSearchResult>& movieResults, const QList<TmdbSearchResult>& tvResults)
    : m_movieResults(movieResults)
    , m_tvResults(tvResults)
{
}

TmdbFindResult TmdbFindResult::fromJson(const QJsonObject& json)
{
    QList<TmdbSearchResult> movieResults;
    QList<TmdbSearchResult> tvResults;
    
    QJsonArray movieArray = json["movie_results"].toArray();
    for (const QJsonValue& value : movieArray) {
        movieResults.append(TmdbSearchResult::fromJson(value.toObject()));
    }
    
    QJsonArray tvArray = json["tv_results"].toArray();
    for (const QJsonValue& value : tvArray) {
        tvResults.append(TmdbSearchResult::fromJson(value.toObject()));
    }
    
    return TmdbFindResult(movieResults, tvResults);
}

QJsonObject TmdbFindResult::toJson() const
{
    QJsonObject obj;
    QJsonArray movieArray;
    for (const TmdbSearchResult& result : m_movieResults) {
        movieArray.append(result.toJson());
    }
    obj["movie_results"] = movieArray;
    
    QJsonArray tvArray;
    for (const TmdbSearchResult& result : m_tvResults) {
        tvArray.append(result.toJson());
    }
    obj["tv_results"] = tvArray;
    return obj;
}





