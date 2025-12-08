#ifndef MEDIA_METADATA_SERVICE_H
#define MEDIA_METADATA_SERVICE_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QMap>
#include <QDateTime>
#include <memory>
#include <QtQmlIntegration/qqmlintegration.h>
#include "interfaces/imedia_metadata_service.h"

class TmdbDataService;
class OmdbService;
class TraktCoreService;
class AddonRepository;
class AddonClient;
struct AddonConfig;

class MediaMetadataService : public QObject, public IMediaMetadataService
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit MediaMetadataService(
        std::shared_ptr<TmdbDataService> tmdbService,
        std::shared_ptr<OmdbService> omdbService,
        std::shared_ptr<AddonRepository> addonRepository,
        TraktCoreService* traktService,
        QObject* parent = nullptr);
    
    Q_INVOKABLE void getCompleteMetadata(const QString& contentId, const QString& type);
    Q_INVOKABLE void getCompleteMetadataFromTmdbId(int tmdbId, const QString& type);
    
    // Get episodes for a series (from cached AIOMetadata response)
    Q_INVOKABLE QVariantList getSeriesEpisodes(const QString& contentId, int seasonNumber = -1);
    QVariantList getSeriesEpisodesByTmdbId(int tmdbId, int seasonNumber);
    
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
    void onAddonMetaFetched(const QString& type, const QString& id, const QJsonObject& meta);
    void onAddonMetaError(const QString& errorMessage);

private:
    std::shared_ptr<TmdbDataService> m_tmdbService;
    std::shared_ptr<OmdbService> m_omdbService;
    std::shared_ptr<AddonRepository> m_addonRepository;
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
    QMap<AddonClient*, QString> m_pendingAddonRequests; // AddonClient -> cacheKey
    QMap<QString, QVariantList> m_seriesEpisodes; // contentId -> episodes list (for series)
    QMap<int, QString> m_tmdbIdToContentId; // TMDB ID -> contentId (for episode lookups)
    
    // Helper methods
    AddonConfig findAiometadataAddon();
    void fetchMetadataFromAddon(const QString& contentId, const QString& type);
    
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


