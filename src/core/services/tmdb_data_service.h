#ifndef TMDB_DATA_SERVICE_H
#define TMDB_DATA_SERVICE_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantList>
#include <memory>
#include <QtQmlIntegration/qqmlintegration.h>
#include "tmdb_api_client.h"
#include "../models/tmdb_models.h"

class TmdbDataService : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit TmdbDataService(std::unique_ptr<TmdbApiClient> apiClient = nullptr, QObject* parent = nullptr);
    
    Q_INVOKABLE void getTmdbIdFromImdb(const QString& imdbId);
    Q_INVOKABLE void getMovieMetadata(int tmdbId);
    Q_INVOKABLE void getTvMetadata(int tmdbId);
    Q_INVOKABLE void getCastAndCrew(int tmdbId, const QString& type);
    Q_INVOKABLE void getSimilarMovies(int tmdbId);
    Q_INVOKABLE void getSimilarTv(int tmdbId);
    Q_INVOKABLE void searchMovies(const QString& query, int page = 1);
    Q_INVOKABLE void searchTv(const QString& query, int page = 1);
    Q_INVOKABLE void getTvSeasonDetails(int tmdbId, int seasonNumber);
    
    // Cache management (delegates to api client)
    void clearCache();
    void clearCacheForId(int tmdbId, const QString& type);
    int getCacheSize() const;

signals:
    void movieMetadataFetched(int tmdbId, const QJsonObject& data);
    void tvMetadataFetched(int tmdbId, const QJsonObject& data);
    void tmdbIdFound(const QString& imdbId, int tmdbId);
    void castAndCrewFetched(int tmdbId, const QString& type, const QJsonObject& data);
    void similarMoviesFetched(int tmdbId, const QJsonArray& results);
    void similarTvFetched(int tmdbId, const QJsonArray& results);
    void moviesFound(const QVariantList& results);
    void tvFound(const QVariantList& results);
    void tvSeasonDetailsFetched(int tmdbId, int seasonNumber, const QJsonObject& data);
    void error(const QString& message);

private slots:
    void onApiClientError(const TmdbErrorInfo& errorInfo);
    void onCachedResponseReady(const QString& path, [[maybe_unused]] const QUrlQuery& query, const QJsonObject& data);
    void onFindReplyFinished();
    void onMovieMetadataReplyFinished();
    void onTvMetadataReplyFinished();
    void onCastAndCrewReplyFinished();
    void onSimilarMoviesReplyFinished();
    void onSimilarTvReplyFinished();
    void onMoviesSearchReplyFinished();
    void onTvSearchReplyFinished();
    void onTvSeasonDetailsReplyFinished();

private:
    std::unique_ptr<TmdbApiClient> m_apiClient;
    QMap<QNetworkReply*, QString> m_replyContexts; // Track reply context for error handling
    
    void handleReply(QNetworkReply* reply, const QString& context);
    static bool compareByPopularity(const TmdbSearchResult& a, const TmdbSearchResult& b);
};

#endif // TMDB_DATA_SERVICE_H


