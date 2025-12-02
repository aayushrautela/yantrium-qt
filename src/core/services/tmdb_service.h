#ifndef TMDB_SERVICE_H
#define TMDB_SERVICE_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QDateTime>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>
#include "../services/id_parser.h"

struct CachedMetadata {
    QJsonObject data;
    QDateTime timestamp;
    static constexpr int ttlSeconds = 300; // 5 minutes
    
    bool isExpired() const {
        return QDateTime::currentDateTime().secsTo(timestamp) < -ttlSeconds;
    }
};

class TmdbService : public QObject
{
    Q_OBJECT

public:
    explicit TmdbService(QObject* parent = nullptr);
    
    Q_INVOKABLE void getTmdbIdFromImdb(const QString& imdbId);
    Q_INVOKABLE void getMovieMetadata(int tmdbId);
    Q_INVOKABLE void getTvMetadata(int tmdbId);
    Q_INVOKABLE void getCastAndCrew(int tmdbId, const QString& type);
    Q_INVOKABLE void getCatalogItemFromContentId(const QString& contentId, const QString& type);
    Q_INVOKABLE void getSimilarMovies(int tmdbId);
    Q_INVOKABLE void getSimilarTv(int tmdbId);
    
    // Synchronous conversion methods (use cached/fetched data)
    static QJsonObject convertMovieToCatalogItem(const QJsonObject& tmdbData, const QString& contentId);
    static QJsonObject convertTvToCatalogItem(const QJsonObject& tmdbData, const QString& contentId);
    
    // Cache management
    void clearCache();
    void clearCacheForId(int tmdbId, const QString& type);
    int getCacheSize() const;

signals:
    void movieMetadataFetched(int tmdbId, const QJsonObject& data);
    void tvMetadataFetched(int tmdbId, const QJsonObject& data);
    void tmdbIdFound(const QString& imdbId, int tmdbId);
    void castAndCrewFetched(int tmdbId, const QString& type, const QJsonObject& data);
    void catalogItemFetched(const QJsonObject& item);
    void similarMoviesFetched(int tmdbId, const QJsonArray& results);
    void similarTvFetched(int tmdbId, const QJsonArray& results);
    void error(const QString& message);

private slots:
    void onFindReplyFinished();
    void onMovieMetadataReplyFinished();
    void onTvMetadataReplyFinished();
    void onCastAndCrewReplyFinished();
    void onSimilarMoviesReplyFinished();
    void onSimilarTvReplyFinished();

private:
    QNetworkAccessManager* m_networkManager;
    QMap<QString, CachedMetadata> m_cache;
    
    QUrl buildUrl(const QString& path, const QUrlQuery& query = QUrlQuery());
    void handleError(QNetworkReply* reply, const QString& context);
    QString getImageUrl(const QString& path, const QString& size = "w500") const;
};

#endif // TMDB_SERVICE_H

