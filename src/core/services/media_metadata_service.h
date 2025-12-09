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
        std::shared_ptr<OmdbService> omdbService,
        std::shared_ptr<AddonRepository> addonRepository,
        TraktCoreService* traktService,
        QObject* parent = nullptr);
    
    Q_INVOKABLE void getCompleteMetadata(const QString& contentId, const QString& type);
    void getCompleteMetadataFromTmdbId(int tmdbId, const QString& type) override;
    
    // Get episodes for a series (from cached AIOMetadata response)
    Q_INVOKABLE QVariantList getSeriesEpisodes(const QString& contentId, int seasonNumber = -1);
    
    // Cache management
    void clearMetadataCache();
    int getMetadataCacheSize() const;

signals:
    void metadataLoaded(const QVariantMap& completeMetadata);
    void error(const QString& message);

private slots:
    void onOmdbRatingsFetched(const QString& imdbId, const QJsonObject& data);
    void onOmdbError(const QString& message, const QString& imdbId);
    void onAddonMetaFetched(const QString& type, const QString& id, const QJsonObject& meta);
    void onAddonMetaError(const QString& errorMessage);

private:
    std::shared_ptr<OmdbService> m_omdbService;
    std::shared_ptr<AddonRepository> m_addonRepository;
    TraktCoreService* m_traktService;
    
    // Track pending requests
    struct PendingRequest {
        QString contentId;
        QString type;
        QVariantMap details;
    };
    
    QMap<QString, PendingRequest> m_pendingDetailsByContentId; // contentId -> pending request
    QMap<AddonClient*, QString> m_pendingAddonRequests; // AddonClient -> cacheKey
    QMap<QString, QVariantList> m_seriesEpisodes; // contentId -> episodes list (for series)
    
    // Helper methods
    AddonConfig findMetadataAddon();  // Prefers AIOMetadata, falls back to any addon with meta resource
    void fetchMetadataFromAddon(const AddonConfig& addon, const QString& contentId, const QString& type);
};

#endif // MEDIA_METADATA_SERVICE_H


