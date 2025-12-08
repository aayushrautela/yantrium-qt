#include "media_metadata_service.h"
#include "cache_service.h"
#include "logging_service.h"
#include "error_service.h"
#include "tmdb_data_service.h"
#include "omdb_service.h"
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
    std::shared_ptr<TmdbDataService> tmdbService,
    std::shared_ptr<OmdbService> omdbService,
    std::shared_ptr<AddonRepository> addonRepository,
    TraktCoreService* traktService,
    QObject* parent)
    : QObject(parent)
    , m_tmdbService(std::move(tmdbService))
    , m_omdbService(std::move(omdbService))
    , m_addonRepository(std::move(addonRepository))
    , m_traktService(traktService ? traktService : &TraktCoreService::instance())
{
    // Connect to TMDB service
    if (m_tmdbService) {
        connect(m_tmdbService.get(), &TmdbDataService::tmdbIdFound,
                this, &MediaMetadataService::onTmdbIdFound);
        connect(m_tmdbService.get(), &TmdbDataService::movieMetadataFetched,
                this, &MediaMetadataService::onTmdbMovieMetadataFetched);
        connect(m_tmdbService.get(), &TmdbDataService::tvMetadataFetched,
                this, &MediaMetadataService::onTmdbTvMetadataFetched);
        connect(m_tmdbService.get(), &TmdbDataService::error,
                this, &MediaMetadataService::onTmdbError);
    }
    
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
        ErrorService::report("Missing contentId or type", "MISSING_PARAMS", "MediaMetadataService");
        emit error("Missing contentId or type");
        return;
    }
    
    // Check cache first
    QString cacheKey = "metadata:" + contentId + "|" + type;
    QVariant cached = CacheService::getCache(cacheKey);
    if (cached.isValid() && cached.canConvert<QVariantMap>()) {
        LoggingService::logDebug("MediaMetadataService", QString("Cache hit for: %1").arg(cacheKey));
        emit metadataLoaded(cached.toMap());
        return;
    }
    
    // Try to fetch from AIOMetadata addon first
    if (m_addonRepository) {
        AddonConfig aiometadataAddon = findAiometadataAddon();
        if (!aiometadataAddon.id.isEmpty()) {
            // Removed debug log - unnecessary
            fetchMetadataFromAddon(contentId, type);
            return;
        }
    }
    
    // If AIOMetadata not available, emit error
    ErrorService::report("AIOMetadata addon not installed or not enabled", "ADDON_ERROR", "MediaMetadataService");
    emit error("AIOMetadata addon not installed or not enabled");
}

void MediaMetadataService::getCompleteMetadataFromTmdbId(int tmdbId, const QString& type)
{
    // Removed debug log - unnecessary
    
    // Convert TMDB ID to contentId format and use the main method
    QString contentId = QString("tmdb:%1").arg(tmdbId);
    getCompleteMetadata(contentId, type);
}

void MediaMetadataService::onTmdbIdFound(const QString& imdbId, int tmdbId)
{
    // Removed debug log - unnecessary
    
    if (!m_pendingDetailsByImdbId.contains(imdbId)) {
        // Removed warning log - unnecessary
        return;
    }
    
    PendingRequest& request = m_pendingDetailsByImdbId[imdbId];
    request.tmdbId = tmdbId;
    m_tmdbIdToImdbId[tmdbId] = imdbId;
    
    if (!m_tmdbService) {
        // Removed warning log - unnecessary
        ErrorService::report("TMDB service not available", "SERVICE_UNAVAILABLE", "MediaMetadataService");
        emit error("TMDB service not available");
        return;
    }
    
    // Fetch metadata based on type
    if (request.type == "movie") {
        m_tmdbService->getMovieMetadata(tmdbId);
    } else if (request.type == "series" || request.type == "tv") {
        m_tmdbService->getTvMetadata(tmdbId);
    }
}

void MediaMetadataService::onTmdbMovieMetadataFetched(int tmdbId, const QJsonObject& data)
{
    // Removed debug log - unnecessary
    
    // Get the original contentId from the mapping (could be "tmdb:123" or IMDB ID)
    QString originalContentId = m_tmdbIdToImdbId.value(tmdbId);
    
    // Extract IMDB ID from TMDB response
    QString imdbId;
    QJsonObject externalIds = data["external_ids"].toObject();
    imdbId = externalIds["imdb_id"].toString();
    
    // Get contentId and type from pending request
    QString contentId = originalContentId;
    QString type = "movie";
    
    // Check if we have a pending request stored with TMDB ID key
    QString tmdbKey = QString("tmdb:%1").arg(tmdbId);
    if (m_pendingDetailsByImdbId.contains(tmdbKey)) {
        PendingRequest& pending = m_pendingDetailsByImdbId[tmdbKey];
        contentId = pending.contentId;
        type = pending.type;
        // Remove the temporary TMDB key entry
        m_pendingDetailsByImdbId.remove(tmdbKey);
    } else if (m_pendingDetailsByImdbId.contains(imdbId)) {
        // If we already have it keyed by IMDB ID (from initial IMDB ID request)
        PendingRequest& pending = m_pendingDetailsByImdbId[imdbId];
        contentId = pending.contentId;
        type = pending.type;
    } else if (!imdbId.isEmpty() && imdbId.startsWith("tt")) {
        // If we extracted IMDB ID but no pending request, use IMDB ID as contentId
        contentId = imdbId;
    } else if (!originalContentId.isEmpty()) {
        // Fallback to original contentId
        contentId = originalContentId;
    }
    
    // Update mapping to use IMDB ID if we have it
    if (!imdbId.isEmpty() && imdbId.startsWith("tt")) {
        m_tmdbIdToImdbId[tmdbId] = imdbId;
    }
    
    QVariantMap details = FrontendDataMapper::mapTmdbToDetailVariantMap(data, contentId, type);
    
    if (details.isEmpty()) {
        ErrorService::report("Failed to convert TMDB movie data to detail map", "CONVERSION_ERROR", "MediaMetadataService");
        emit error("Failed to convert TMDB movie data to detail map");
        return;
    }
    
    // Fetch OMDB ratings if we have an IMDB ID and API key is configured
    Configuration& config = Configuration::instance();
    if (!imdbId.isEmpty() && imdbId.startsWith("tt") && !config.omdbApiKey().isEmpty()) {
        if (!m_omdbService) {
            // Removed warning log - unnecessary
            // Cache and emit metadata without OMDB ratings
            QString cacheKey = contentId + "|" + type;
            CacheService::setCache(cacheKey, QVariant::fromValue(details), 3600); // 1 hour TTL
            emit metadataLoaded(details);
            return;
        }
        // Store details keyed by IMDB ID for matching when OMDB response arrives
        m_pendingDetailsByImdbId[imdbId] = {contentId, type, tmdbId, details};
        // Removed debug logs - unnecessary
        m_omdbService->getRatings(imdbId);
    } else {
        // No IMDB ID or no API key, emit details without OMDB ratings
        if (imdbId.isEmpty() || !imdbId.startsWith("tt")) {
            // Removed debug logs - unnecessary
        } else {
            // Removed debug logs - unnecessary
        }
        
        // Cache and emit metadata
        QString cacheKey = "metadata:" + contentId + "|" + type;
        CacheService::setCache(cacheKey, QVariant::fromValue(details), 3600); // 1 hour TTL
        
        emit metadataLoaded(details);
    }
}

void MediaMetadataService::onTmdbTvMetadataFetched(int tmdbId, const QJsonObject& data)
{
    // Removed debug log - unnecessary
    
    // Get the original contentId from the mapping (could be "tmdb:123" or IMDB ID)
    QString originalContentId = m_tmdbIdToImdbId.value(tmdbId);
    
    // Extract IMDB ID from TMDB response
    QString imdbId;
    QJsonObject externalIds = data["external_ids"].toObject();
    imdbId = externalIds["imdb_id"].toString();
    
    // Get contentId and type from pending request
    QString contentId = originalContentId;
    QString type = "series";
    
    // Check if we have a pending request stored with TMDB ID key
    QString tmdbKey = QString("tmdb:%1").arg(tmdbId);
    if (m_pendingDetailsByImdbId.contains(tmdbKey)) {
        PendingRequest& pending = m_pendingDetailsByImdbId[tmdbKey];
        contentId = pending.contentId;
        type = pending.type;
        // Remove the temporary TMDB key entry
        m_pendingDetailsByImdbId.remove(tmdbKey);
    } else if (m_pendingDetailsByImdbId.contains(imdbId)) {
        // If we already have it keyed by IMDB ID (from initial IMDB ID request)
        PendingRequest& pending = m_pendingDetailsByImdbId[imdbId];
        contentId = pending.contentId;
        type = pending.type;
    } else if (!imdbId.isEmpty() && imdbId.startsWith("tt")) {
        // If we extracted IMDB ID but no pending request, use IMDB ID as contentId
        contentId = imdbId;
    } else if (!originalContentId.isEmpty()) {
        // Fallback to original contentId
        contentId = originalContentId;
    }
    
    // Update mapping to use IMDB ID if we have it
    if (!imdbId.isEmpty() && imdbId.startsWith("tt")) {
        m_tmdbIdToImdbId[tmdbId] = imdbId;
    }
    
    QVariantMap details = FrontendDataMapper::mapTmdbToDetailVariantMap(data, contentId, type);
    
    if (details.isEmpty()) {
        ErrorService::report("Failed to convert TMDB TV data to detail map", "CONVERSION_ERROR", "MediaMetadataService");
        emit error("Failed to convert TMDB TV data to detail map");
        return;
    }
    
    // Fetch OMDB ratings if we have an IMDB ID and API key is configured
    Configuration& config = Configuration::instance();
    if (!imdbId.isEmpty() && imdbId.startsWith("tt") && !config.omdbApiKey().isEmpty()) {
        if (!m_omdbService) {
            // Removed warning log - unnecessary
            emit metadataLoaded(details);
            return;
        }
        // Store details keyed by IMDB ID for matching when OMDB response arrives
        m_pendingDetailsByImdbId[imdbId] = {contentId, type, tmdbId, details};
        // Removed debug logs - unnecessary
        m_omdbService->getRatings(imdbId);
    } else {
        // No IMDB ID or no API key, emit details without OMDB ratings
        if (imdbId.isEmpty() || !imdbId.startsWith("tt")) {
            // Removed debug logs - unnecessary
        } else {
            // Removed debug logs - unnecessary
        }
        emit metadataLoaded(details);
    }
}

void MediaMetadataService::onOmdbRatingsFetched(const QString& imdbId, const QJsonObject& data)
{
    if (!m_pendingDetailsByImdbId.contains(imdbId)) {
        // Removed warning logs - unnecessary
        // This shouldn't happen, but if it does, we can't merge ratings
        // The details were likely already emitted without OMDB ratings
        return;
    }
    
    PendingRequest& request = m_pendingDetailsByImdbId[imdbId];
    QVariantMap details = request.details;
    
    // Removed debug log - unnecessary
    
    // Merge OMDB ratings into details
    FrontendDataMapper::mergeOmdbRatings(details, data);
    
    // Cache and emit complete metadata
    QString cacheKey = "metadata:" + request.contentId + "|" + request.type;
    CacheService::setCache(cacheKey, QVariant::fromValue(details), 3600); // 1 hour TTL
    
    emit metadataLoaded(details);
    
    // Clean up
    m_pendingDetailsByImdbId.remove(imdbId);
}

void MediaMetadataService::onOmdbError(const QString& /*message*/, const QString& imdbId)
{
    // Removed debug log - unnecessary
    
    // Even if OMDB fails, emit the details we have from TMDB
    if (m_pendingDetailsByImdbId.contains(imdbId)) {
        PendingRequest& request = m_pendingDetailsByImdbId[imdbId];
        
        // Cache and emit metadata
        QString cacheKey = "metadata:" + request.contentId + "|" + request.type;
        CacheService::setCache(cacheKey, QVariant::fromValue(request.details), 3600); // 1 hour TTL
        
        emit metadataLoaded(request.details);
        m_pendingDetailsByImdbId.remove(imdbId);
    }
}

void MediaMetadataService::onTmdbError(const QString& message)
{
    // Removed warning log - unnecessary
    ErrorService::report(message, "MEDIA_METADATA_ERROR", "MediaMetadataService");
    emit error(message);
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
    return CacheService::instance().size();
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

QVariantList MediaMetadataService::getSeriesEpisodesByTmdbId(int tmdbId, int seasonNumber)
{
    // Removed debug log - unnecessary
    
    // Look up contentId from TMDB ID
    if (!m_tmdbIdToContentId.contains(tmdbId)) {
        // Removed debug log - unnecessary
        return QVariantList();
    }
    
    QString contentId = m_tmdbIdToContentId[tmdbId];
    return getSeriesEpisodes(contentId, seasonNumber);
}

AddonConfig MediaMetadataService::findAiometadataAddon()
{
    if (!m_addonRepository) {
        return AddonConfig();
    }
    
    QList<AddonConfig> enabledAddons = m_addonRepository->getEnabledAddons();
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
            // Removed debug log - unnecessary
            return addon;
        }
    }
    
    // Removed debug log - unnecessary
    return AddonConfig();
}

void MediaMetadataService::fetchMetadataFromAddon(const QString& contentId, const QString& type)
{
    AddonConfig addon = findAiometadataAddon();
    if (addon.id.isEmpty()) {
        ErrorService::report("AIOMetadata addon not found", "ADDON_ERROR", "MediaMetadataService");
        emit error("AIOMetadata addon not found");
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
    request.tmdbId = 0;
    m_pendingDetailsByImdbId[cacheKey] = request;
    
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
        qWarning() << "[MediaMetadataService] Received meta from unknown client";
        return;
    }
    
    if (!m_pendingAddonRequests.contains(client)) {
        // Removed warning log - unnecessary
        client->deleteLater();
        return;
    }
    
    QString cacheKey = m_pendingAddonRequests.take(client);
    
    if (!m_pendingDetailsByImdbId.contains(cacheKey)) {
        // Removed warning log - unnecessary
        client->deleteLater();
        return;
    }
    
    PendingRequest& request = m_pendingDetailsByImdbId[cacheKey];
    
    // Normalize type back (Stremio uses "series", we use "tv")
    QString normalizedType = (type == "series") ? "tv" : type;
    
    // Extract the "meta" object from the response
    // AIOMetadata wraps metadata in a "meta" key: {"meta": {...}}
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
        ErrorService::report("Failed to convert addon metadata to detail map", "CONVERSION_ERROR", "MediaMetadataService");
        emit error("Failed to convert addon metadata to detail map");
        if (m_pendingAddonRequests.contains(client)) {
            m_pendingAddonRequests.remove(client);
        }
        client->deleteLater();
        return;
    }
    
    // Extract episodes from videos array for series (AIOMetadata stores episodes in videos)
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
        // Store mapping from TMDB ID to contentId for episode lookups
        QString tmdbIdStr = details["tmdbId"].toString();
        if (!tmdbIdStr.isEmpty()) {
            bool ok;
            int tmdbId = tmdbIdStr.toInt(&ok);
            if (ok && tmdbId > 0) {
                m_tmdbIdToContentId[tmdbId] = request.contentId;
            }
        }
    }
    
    // Cache and emit metadata
    QString cacheKeyForCache = "metadata:" + request.contentId + "|" + normalizedType;
    CacheService::setCache(cacheKeyForCache, QVariant::fromValue(details), 3600); // 1 hour TTL
    
    emit metadataLoaded(details);
    
    // Clean up
    m_pendingDetailsByImdbId.remove(cacheKey);
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
        m_pendingDetailsByImdbId.remove(cacheKey);
    }
    
    QString errorMsg = QString("Failed to fetch metadata from AIOMetadata: %1").arg(errorMessage);
    ErrorService::report(errorMsg, "ADDON_ERROR", "MediaMetadataService");
    emit error(errorMsg);
    
    client->deleteLater();
}


