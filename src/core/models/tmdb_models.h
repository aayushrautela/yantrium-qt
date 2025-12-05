#ifndef TMDB_MODELS_H
#define TMDB_MODELS_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>

// ----------------------------------------------------------------------------
// TmdbIds
// ----------------------------------------------------------------------------
struct TmdbIds
{
    QString imdbId;
    int tmdbId = 0;

    static TmdbIds fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

// ----------------------------------------------------------------------------
// TmdbGenre
// ----------------------------------------------------------------------------
struct TmdbGenre
{
    int id = 0;
    QString name;

    static TmdbGenre fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

// ----------------------------------------------------------------------------
// TmdbProductionCompany
// ----------------------------------------------------------------------------
struct TmdbProductionCompany
{
    int id = 0;
    QString name;
    QString logoPath;
    QString originCountry;

    static TmdbProductionCompany fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

// ----------------------------------------------------------------------------
// TmdbSpokenLanguage
// ----------------------------------------------------------------------------
struct TmdbSpokenLanguage
{
    QString iso6391;
    QString name;

    static TmdbSpokenLanguage fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

// ----------------------------------------------------------------------------
// TmdbSearchResult
// ----------------------------------------------------------------------------
struct TmdbSearchResult
{
    int id = 0;
    QString title;
    QString name;
    QString overview;
    QString releaseDate;
    QString firstAirDate;
    QString posterPath;
    QString backdropPath;
    QString mediaType;
    
    double voteAverage = 0.0;
    double popularity = 0.0;
    int voteCount = 0;
    bool adult = false;

    static TmdbSearchResult fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

// ----------------------------------------------------------------------------
// TmdbFindResult
// ----------------------------------------------------------------------------
struct TmdbFindResult
{
    QList<TmdbSearchResult> movieResults;
    QList<TmdbSearchResult> tvResults;

    static TmdbFindResult fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

#endif // TMDB_MODELS_H