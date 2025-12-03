#ifndef MEDIA_METADATA_SERVICE_H
#define MEDIA_METADATA_SERVICE_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QMap>
#include <QDateTime>

class TmdbDataService;
class OmdbService;
class TraktCoreService;

class MediaMetadataService : public QObject
{
    Q_OBJECT

public:
    explicit MediaMetadataService(QObject* parent = nullptr);
    
    Q_INVOKABLE void getCompleteMetadata(const QString& contentId, const QString& type);
    Q_INVOKABLE void getCompleteMetadataFromTmdbId(int tmdbId, const QString& type);
    
    // Cache management
    void clearMetadataCache();
    int getMetadataCacheSize() const;

signals:
    void metadataLoaded(const QVariantMap& completeMetadata);
    void error(const QString& message);

private slots:
    void onTmdbIdFound(const QString& imdbId, int tmdbId);
    void onTmdbMovieMetadataFetched(int tmdbId, const QJsonObject& data);
    void onTmdbTvMetadataFetched(int tmdbId, const QJsonObject& data);
    void onTmdbError(const QString& message);
    void onOmdbRatingsFetched(const QString& imdbId, const QJsonObject& data);
    void onOmdbError(const QString& message, const QString& imdbId);

private:
    TmdbDataService* m_tmdbService;
    OmdbService* m_omdbService;
    TraktCoreService* m_traktService;
    
    // Track pending requests
    struct PendingRequest {
        QString contentId;
        QString type;
        int tmdbId;
        QVariantMap details;
    };
    
    QMap<QString, PendingRequest> m_pendingDetailsByImdbId; // IMDB ID -> pending request
    QMap<int, QString> m_tmdbIdToImdbId; // TMDB ID -> IMDB ID
    
    // Metadata cache
    struct CachedMetadata {
        QVariantMap data;
        QDateTime timestamp;
        static constexpr int ttlSeconds = 3600; // 1 hour
        
        bool isExpired() const {
            return QDateTime::currentDateTime().secsTo(timestamp) < -ttlSeconds;
        }
    };
    QMap<QString, CachedMetadata> m_metadataCache;
    
    void processPendingRequest(const QString& imdbId);
};

#endif // MEDIA_METADATA_SERVICE_H


