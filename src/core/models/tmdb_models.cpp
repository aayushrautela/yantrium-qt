#include "tmdb_models.h"

// ----------------------------------------------------------------------------
// TmdbIds
// ----------------------------------------------------------------------------
TmdbIds TmdbIds::fromJson(const QJsonObject& json)
{
    TmdbIds obj;
    obj.imdbId = json["imdb_id"].toString();
    obj.tmdbId = json["id"].toInt();
    return obj;
}

QJsonObject TmdbIds::toJson() const
{
    QJsonObject obj;
    if (!imdbId.isEmpty()) obj["imdb_id"] = imdbId;
    if (tmdbId > 0)        obj["id"] = tmdbId;
    return obj;
}

// ----------------------------------------------------------------------------
// TmdbGenre
// ----------------------------------------------------------------------------
TmdbGenre TmdbGenre::fromJson(const QJsonObject& json)
{
    TmdbGenre obj;
    obj.id = json["id"].toInt(0);
    obj.name = json["name"].toString();
    return obj;
}

QJsonObject TmdbGenre::toJson() const
{
    QJsonObject obj;
    obj["id"] = id;
    obj["name"] = name;
    return obj;
}

// ----------------------------------------------------------------------------
// TmdbProductionCompany
// ----------------------------------------------------------------------------
TmdbProductionCompany TmdbProductionCompany::fromJson(const QJsonObject& json)
{
    TmdbProductionCompany obj;
    obj.id = json["id"].toInt(0);
    obj.name = json["name"].toString();
    obj.logoPath = json["logo_path"].toString();
    obj.originCountry = json["origin_country"].toString();
    return obj;
}

QJsonObject TmdbProductionCompany::toJson() const
{
    QJsonObject obj;
    obj["id"] = id;
    obj["name"] = name;
    if (!logoPath.isEmpty())      obj["logo_path"] = logoPath;
    if (!originCountry.isEmpty()) obj["origin_country"] = originCountry;
    return obj;
}

// ----------------------------------------------------------------------------
// TmdbSpokenLanguage
// ----------------------------------------------------------------------------
TmdbSpokenLanguage TmdbSpokenLanguage::fromJson(const QJsonObject& json)
{
    TmdbSpokenLanguage obj;
    obj.iso6391 = json["iso_639_1"].toString();
    obj.name = json["name"].toString();
    return obj;
}

QJsonObject TmdbSpokenLanguage::toJson() const
{
    QJsonObject obj;
    obj["iso_639_1"] = iso6391;
    obj["name"] = name;
    return obj;
}

// ----------------------------------------------------------------------------
// TmdbSearchResult
// ----------------------------------------------------------------------------
TmdbSearchResult TmdbSearchResult::fromJson(const QJsonObject& json)
{
    TmdbSearchResult obj;
    obj.id = json["id"].toInt(0);
    obj.title = json["title"].toString();
    obj.name = json["name"].toString();
    obj.overview = json["overview"].toString();
    obj.releaseDate = json["release_date"].toString();
    obj.firstAirDate = json["first_air_date"].toString();
    obj.posterPath = json["poster_path"].toString();
    obj.backdropPath = json["backdrop_path"].toString();
    obj.voteAverage = json["vote_average"].toDouble(0.0);
    obj.voteCount = json["vote_count"].toInt(0);
    obj.popularity = json["popularity"].toDouble(0.0);
    obj.adult = json["adult"].toBool(false);
    obj.mediaType = json["media_type"].toString();
    return obj;
}

QJsonObject TmdbSearchResult::toJson() const
{
    QJsonObject obj;
    obj["id"] = id;
    if (!title.isEmpty())        obj["title"] = title;
    if (!name.isEmpty())         obj["name"] = name;
    if (!overview.isEmpty())     obj["overview"] = overview;
    if (!releaseDate.isEmpty())  obj["release_date"] = releaseDate;
    if (!firstAirDate.isEmpty()) obj["first_air_date"] = firstAirDate;
    if (!posterPath.isEmpty())   obj["poster_path"] = posterPath;
    if (!backdropPath.isEmpty()) obj["backdrop_path"] = backdropPath;
    obj["vote_average"] = voteAverage;
    obj["vote_count"] = voteCount;
    obj["popularity"] = popularity;
    obj["adult"] = adult;
    if (!mediaType.isEmpty())    obj["media_type"] = mediaType;
    return obj;
}

// ----------------------------------------------------------------------------
// TmdbFindResult
// ----------------------------------------------------------------------------
TmdbFindResult TmdbFindResult::fromJson(const QJsonObject& json)
{
    TmdbFindResult obj;

    const QJsonArray movieArray = json["movie_results"].toArray();
    for (const QJsonValue& value : movieArray) {
        obj.movieResults.append(TmdbSearchResult::fromJson(value.toObject()));
    }

    const QJsonArray tvArray = json["tv_results"].toArray();
    for (const QJsonValue& value : tvArray) {
        obj.tvResults.append(TmdbSearchResult::fromJson(value.toObject()));
    }

    return obj;
}

QJsonObject TmdbFindResult::toJson() const
{
    QJsonObject obj;
    
    QJsonArray movieArray;
    for (const TmdbSearchResult& result : movieResults) {
        movieArray.append(result.toJson());
    }
    obj["movie_results"] = movieArray;

    QJsonArray tvArray;
    for (const TmdbSearchResult& result : tvResults) {
        tvArray.append(result.toJson());
    }
    obj["tv_results"] = tvArray;

    return obj;
}