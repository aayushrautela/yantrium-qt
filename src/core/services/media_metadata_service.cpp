#include "media_metadata_service.h"
#include "cache_service.h"
#include "logging_service.h"
#include "logging_service.h"
#include "omdb_service.h"
#include "core/di/service_registry.h"
#include "trakt_core_service.h"
#include "frontend_data_mapper.h"
#include "id_parser.h"
#include "configuration.h"
#include "features/addons/logic/addon_repository.h"
#include "features/addons/logic/addon_client.h"
#include "features/addons/models/addon_config.h"
#include "features/addons/models/addon_manifest.h"
#include <QJsonObject>
#include <QDateTime>
#include <algorithm>

MediaMetadataService::MediaMetadataService(
    std::shared_ptr<OmdbService> omdbService,
    std::shared_ptr<AddonRepository> addonRepository,
    TraktCoreService* traktService,
    QObject* parent)
    : QObject(parent)
    , m_omdbService(std::move(omdbService))
    , m_addonRepository(std::move(addonRepository))
    , m_traktService(traktService)
{
    // Connect to OMDB service
    if (m_omdbService) {
        connect(m_omdbService.get(), &OmdbService::ratingsFetched,
                this, &MediaMetadataService::onOmdbRatingsFetched);
        connect(m_omdbService.get(), &OmdbService::error,
                this, &MediaMetadataService::onOmdbError);
    }
}

void MediaMetadataService::getCompleteMetadata(const QString& contentId, const QString& type)
{
    // Removed debug log - unnecessary
    
    if (contentId.isEmpty() || type.isEmpty()) {
        LoggingService::report("Missing contentId or type", "MISSING_PARAMS", "MediaMetadataService");
        emit error("Missing contentId or type");
        return;
    }
    
    // Check cache first
    QString cacheKey = "metadata:" + contentId + "|" + type;
    QVariant cached = CacheService::instance().get(cacheKey);
    if (cached.isValid() && cached.canConvert<QVariantMap>()) {
        LoggingService::logDebug("MediaMetadataService", QString("Cache hit for: %1").arg(cacheKey));
        emit metadataLoaded(cached.toMap());
        return;
    }
    
    // Try to fetch from addon - prefer AIOMetadata if available, otherwise use any addon with meta resource
    if (m_addonRepository) {
        AddonConfig metadataAddon = findMetadataAddon();
        if (!metadataAddon.id.isEmpty()) {
            fetchMetadataFromAddon(metadataAddon, contentId, type);
            return;
        }
    }
    
    // If no metadata addon available, emit error
    LoggingService::report("No metadata addon available (addon with 'meta' resource not installed or not enabled)", "ADDON_ERROR", "MediaMetadataService");
    emit error("No metadata addon available");
}

void MediaMetadataService::onOmdbRatingsFetched(const QString& imdbId, const QJsonObject& data)
{
    if (!m_pendingDetailsByContentId.contains(imdbId)) {
        // This shouldn't happen, but if it does, we can't merge ratings
        // The details were likely already emitted without OMDB ratings
        return;
    }
    
    PendingRequest& request = m_pendingDetailsByContentId[imdbId];
    QVariantMap details = request.details;
    
    // Merge OMDB ratings into details
    FrontendDataMapper::mergeOmdbRatings(details, data);
    
    // Cache and emit complete metadata
    QString cacheKey = "metadata:" + request.contentId + "|" + request.type;
    CacheService::instance().set(cacheKey, QVariant::fromValue(details), 3600); // 1 hour TTL
    
    emit metadataLoaded(details);
    
    // Clean up
    m_pendingDetailsByContentId.remove(imdbId);
}

void MediaMetadataService::onOmdbError(const QString& /*message*/, const QString& imdbId)
{
    // Even if OMDB fails, emit the details we have from addon
    if (m_pendingDetailsByContentId.contains(imdbId)) {
        PendingRequest& request = m_pendingDetailsByContentId[imdbId];
        
        // Cache and emit metadata
        QString cacheKey = "metadata:" + request.contentId + "|" + request.type;
        CacheService::instance().set(cacheKey, QVariant::fromValue(request.details), 3600); // 1 hour TTL
        
        emit metadataLoaded(request.details);
        m_pendingDetailsByContentId.remove(imdbId);
    }
}

void MediaMetadataService::clearMetadataCache()
{
    // Clear metadata cache entries (all entries with "metadata:" prefix)
    // Note: CacheService doesn't support prefix-based clearing yet
    // For now, we'll clear all cache (can be optimized later)
    CacheService::instance().clear();
    LoggingService::logInfo("MediaMetadataService", "Metadata cache cleared");
}

int MediaMetadataService::getMetadataCacheSize() const
{
    auto cacheService = ServiceRegistry::instance().resolve<CacheService>();
    return cacheService ? cacheService->size() : 0;
}

QVariantList MediaMetadataService::getSeriesEpisodes(const QString& contentId, int seasonNumber)
{
    // Removed debug log - unnecessary
    
    if (!m_seriesEpisodes.contains(contentId)) {
        // Removed debug log - unnecessary
        return QVariantList();
    }
    
    QVariantList allEpisodes = m_seriesEpisodes[contentId];
    
    // If seasonNumber is -1, return all episodes
    if (seasonNumber < 0) {
        return allEpisodes;
    }
    
    // Filter by season number and sort by episode number
    QVariantList seasonEpisodes;
    for (const QVariant& episodeVar : allEpisodes) {
        QVariantMap episode = episodeVar.toMap();
        int episodeSeason = episode["season"].toInt();
        if (episodeSeason == seasonNumber) {
            seasonEpisodes.append(episode);
        }
    }
    
    // Sort by episode number
    std::sort(seasonEpisodes.begin(), seasonEpisodes.end(), [](const QVariant& a, const QVariant& b) {
        QVariantMap aMap = a.toMap();
        QVariantMap bMap = b.toMap();
        return aMap["episodeNumber"].toInt() < bMap["episodeNumber"].toInt();
    });
    
    // Removed debug log - unnecessary
    return seasonEpisodes;
}


AddonConfig MediaMetadataService::findMetadataAddon()
{
    if (!m_addonRepository) {
        return AddonConfig();
    }
    
    QList<AddonConfig> enabledAddons = m_addonRepository->getEnabledAddons();
    AddonConfig aiometadataAddon;  // Prefer AIOMetadata if available
    AddonConfig fallbackAddon;     // Fall back to any addon with meta resource
    
    for (const AddonConfig& addon : enabledAddons) {
        AddonManifest manifest = m_addonRepository->getManifest(addon);
        
        // Check if addon has "meta" resource
        if (!AddonRepository::hasResource(manifest.resources(), "meta")) {
            continue;
        }
        
        // Check if it's AIOMetadata by ID or name
        QString idLower = addon.id.toLower();
        QString nameLower = addon.name.toLower();
        if (idLower.contains("aiometadata") || nameLower.contains("aiometadata")) {
            aiometadataAddon = addon;
            break;  // Found AIOMetadata, prefer it
        } else if (fallbackAddon.id.isEmpty()) {
            // Store first addon with meta resource as fallback
            fallbackAddon = addon;
        }
    }
    
    // Return AIOMetadata if available, otherwise fallback to any metadata addon
    return !aiometadataAddon.id.isEmpty() ? aiometadataAddon : fallbackAddon;
}

void MediaMetadataService::fetchMetadataFromAddon(const AddonConfig& addon, const QString& contentId, const QString& type)
{
    if (addon.id.isEmpty()) {
        LoggingService::report("Metadata addon not found", "ADDON_ERROR", "MediaMetadataService");
        emit error("Metadata addon not found");
        return;
    }
    
    QString baseUrl = AddonClient::extractBaseUrl(addon.manifestUrl);
    AddonClient* client = new AddonClient(baseUrl, this);
    
    // Store cache key for this request
    QString cacheKey = contentId + "|" + type;
    m_pendingAddonRequests[client] = cacheKey;
    
    // Store pending request
    PendingRequest request;
    request.contentId = contentId;
    request.type = type;
    m_pendingDetailsByContentId[cacheKey] = request;
    
    // Connect signals
    connect(client, &AddonClient::metaFetched, this, &MediaMetadataService::onAddonMetaFetched);
    connect(client, &AddonClient::error, this, &MediaMetadataService::onAddonMetaError);
    
    // Normalize type for Stremio (use "series" instead of "tv")
    QString stremioType = (type == "tv") ? "series" : type;
    
    // Removed debug log - unnecessary
    client->getMeta(stremioType, contentId);
}

void MediaMetadataService::onAddonMetaFetched(const QString& type, const QString& id, const QJsonObject& response)
{
    Q_UNUSED(id);
    
    AddonClient* client = qobject_cast<AddonClient*>(sender());
    if (!client) {
        LoggingService::logWarning("MediaMetadataService", "Received meta from unknown client");
        return;
    }
    
    if (!m_pendingAddonRequests.contains(client)) {
        // Removed warning log - unnecessary
        client->deleteLater();
        return;
    }
    
    QString cacheKey = m_pendingAddonRequests.take(client);
    
    if (!m_pendingDetailsByContentId.contains(cacheKey)) {
        // Removed warning log - unnecessary
        client->deleteLater();
        return;
    }
    
    PendingRequest& request = m_pendingDetailsByContentId[cacheKey];
    
    // Normalize type back (Stremio uses "series", we use "tv")
    QString normalizedType = (type == "series") ? "tv" : type;
    
    // Extract the "meta" object from the response
    // Some addons wrap metadata in a "meta" key: {"meta": {...}}
    QJsonObject meta;
    if (response.contains("meta") && response["meta"].isObject()) {
        meta = response["meta"].toObject();
    } else {
        // Fallback: use the response directly if it doesn't have a "meta" wrapper
        meta = response;
    }
    
    // Map Stremio metadata format to our detail format
    QVariantMap details = FrontendDataMapper::mapAddonMetaToDetailVariantMap(meta, request.contentId, normalizedType);
    
    if (details.isEmpty()) {
        LoggingService::report("Failed to convert addon metadata to detail map", "CONVERSION_ERROR", "MediaMetadataService");
        emit error("Failed to convert addon metadata to detail map");
        if (m_pendingAddonRequests.contains(client)) {
            m_pendingAddonRequests.remove(client);
        }
        client->deleteLater();
        return;
    }
    
    // Extract episodes from videos array for series (some addons store episodes in videos)
    if (normalizedType == "tv" || normalizedType == "series") {
        QVariantList episodes;
        if (meta.contains("videos") && meta["videos"].isArray()) {
            QJsonArray videosArray = meta["videos"].toArray();
            for (const QJsonValue& value : videosArray) {
                if (value.isObject()) {
                    QJsonObject videoObj = value.toObject();
                    // Check if it's an episode (has season and episode numbers)
                    if (videoObj.contains("season") && videoObj.contains("episode")) {
                        QVariantMap episode;
                        episode["season"] = videoObj["season"].toInt();
                        episode["episodeNumber"] = videoObj["episode"].toInt();
                        episode["title"] = videoObj["title"].toString();
                        episode["description"] = videoObj["overview"].toString();
                        episode["airDate"] = videoObj["released"].toString();
                        episode["thumbnailUrl"] = videoObj["thumbnail"].toString();
                        // Extract duration if available
                        if (videoObj.contains("runtime")) {
                            episode["duration"] = videoObj["runtime"].toInt();
                        }
                        episodes.append(episode);
                    }
                }
            }
        }
        // Store episodes for this series
        m_seriesEpisodes[request.contentId] = episodes;
    }
    
    // Cache and emit metadata
    QString cacheKeyForCache = "metadata:" + request.contentId + "|" + normalizedType;
    CacheService::instance().set(cacheKeyForCache, QVariant::fromValue(details), 3600); // 1 hour TTL
    
    emit metadataLoaded(details);
    
    // Clean up
    m_pendingDetailsByContentId.remove(cacheKey);
    if (m_pendingAddonRequests.contains(client)) {
        m_pendingAddonRequests.remove(client);
    }
    client->deleteLater();
}

void MediaMetadataService::onAddonMetaError(const QString& errorMessage)
{
    AddonClient* client = qobject_cast<AddonClient*>(sender());
    if (!client) {
        // Removed warning log - unnecessary
        return;
    }
    
    if (!m_pendingAddonRequests.contains(client)) {
        // Removed warning log - unnecessary
        client->deleteLater();
        return;
    }
    
    QString cacheKey = m_pendingAddonRequests.take(client);
    if (!cacheKey.isEmpty()) {
        m_pendingDetailsByContentId.remove(cacheKey);
    }
    
    QString errorMsg = QString("Failed to fetch metadata from addon: %1").arg(errorMessage);
    LoggingService::report(errorMsg, "ADDON_ERROR", "MediaMetadataService");
    emit error(errorMsg);
    
    client->deleteLater();
}

void MediaMetadataService::getCompleteMetadataFromTmdbId(int tmdbId, const QString& type)
{
    // Convert TMDB ID to content ID format and call the main method
    QString contentId = QString("tmdb:%1").arg(tmdbId);
    getCompleteMetadata(contentId, type);
}

