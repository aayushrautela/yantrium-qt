#ifndef TMDB_MODELS_H
#define TMDB_MODELS_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>

/// TMDB ID pair (IMDB/TMDB)
class TmdbIds
{
public:
    TmdbIds();
    TmdbIds(const QString& imdb, int tmdb);
    
    static TmdbIds fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    QString imdb() const { return m_imdb; }
    int tmdb() const { return m_tmdb; }

private:
    QString m_imdb;
    int m_tmdb;
};

/// TMDB Genre
class TmdbGenre
{
public:
    TmdbGenre();
    TmdbGenre(int id, const QString& name);
    
    static TmdbGenre fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    int id() const { return m_id; }
    QString name() const { return m_name; }

private:
    int m_id;
    QString m_name;
};

/// TMDB Production Company
class TmdbProductionCompany
{
public:
    TmdbProductionCompany();
    TmdbProductionCompany(int id, const QString& name, const QString& logoPath = QString(), const QString& originCountry = QString());
    
    static TmdbProductionCompany fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    int id() const { return m_id; }
    QString name() const { return m_name; }
    QString logoPath() const { return m_logoPath; }
    QString originCountry() const { return m_originCountry; }

private:
    int m_id;
    QString m_name;
    QString m_logoPath;
    QString m_originCountry;
};

/// TMDB Spoken Language
class TmdbSpokenLanguage
{
public:
    TmdbSpokenLanguage();
    TmdbSpokenLanguage(const QString& iso6391, const QString& name);
    
    static TmdbSpokenLanguage fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    QString iso6391() const { return m_iso6391; }
    QString name() const { return m_name; }

private:
    QString m_iso6391;
    QString m_name;
};

/// TMDB Search Result
class TmdbSearchResult
{
public:
    TmdbSearchResult();
    TmdbSearchResult(int id, const QString& title, const QString& name, const QString& overview,
                     const QString& releaseDate, const QString& firstAirDate,
                     const QString& posterPath, const QString& backdropPath,
                     double voteAverage, int voteCount, double popularity, bool adult,
                     const QString& mediaType = QString());
    
    static TmdbSearchResult fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    int id() const { return m_id; }
    QString title() const { return m_title; }
    QString name() const { return m_name; }
    QString overview() const { return m_overview; }
    QString releaseDate() const { return m_releaseDate; }
    QString firstAirDate() const { return m_firstAirDate; }
    QString posterPath() const { return m_posterPath; }
    QString backdropPath() const { return m_backdropPath; }
    double voteAverage() const { return m_voteAverage; }
    int voteCount() const { return m_voteCount; }
    double popularity() const { return m_popularity; }
    bool adult() const { return m_adult; }
    QString mediaType() const { return m_mediaType; }

private:
    int m_id;
    QString m_title;
    QString m_name;
    QString m_overview;
    QString m_releaseDate;
    QString m_firstAirDate;
    QString m_posterPath;
    QString m_backdropPath;
    double m_voteAverage;
    int m_voteCount;
    double m_popularity;
    bool m_adult;
    QString m_mediaType;
};

/// TMDB Find Result (for IMDB to TMDB lookup)
class TmdbFindResult
{
public:
    TmdbFindResult();
    TmdbFindResult(const QList<TmdbSearchResult>& movieResults, const QList<TmdbSearchResult>& tvResults);
    
    static TmdbFindResult fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    QList<TmdbSearchResult> movieResults() const { return m_movieResults; }
    QList<TmdbSearchResult> tvResults() const { return m_tvResults; }

private:
    QList<TmdbSearchResult> m_movieResults;
    QList<TmdbSearchResult> m_tvResults;
};

#endif // TMDB_MODELS_H




