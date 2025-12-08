#include "media_metadata_service.h"
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
#include <QDebug>
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
    qDebug() << "[MediaMetadataService] getCompleteMetadata called - contentId:" << contentId << "type:" << type;
    
    if (contentId.isEmpty() || type.isEmpty()) {
        emit error("Missing contentId or type");
        return;
    }
    
    // Check cache first
    QString cacheKey = contentId + "|" + type;
    if (m_metadataCache.contains(cacheKey)) {
        const CachedMetadata& cached = m_metadataCache[cacheKey];
        if (!cached.isExpired()) {
            qDebug() << "[MediaMetadataService] Cache hit for:" << cacheKey;
            emit metadataLoaded(cached.data);
            return;
        } else {
            // Remove expired cache entry
            m_metadataCache.remove(cacheKey);
        }
    }
    
    // Try to fetch from AIOMetadata addon first
    if (m_addonRepository) {
        AddonConfig aiometadataAddon = findAiometadataAddon();
        if (!aiometadataAddon.id.isEmpty()) {
            qDebug() << "[MediaMetadataService] AIOMetadata addon found, fetching metadata from addon";
            fetchMetadataFromAddon(contentId, type);
            return;
        }
    }
    
    // If AIOMetadata not available, emit error
    emit error("AIOMetadata addon not installed or not enabled");
}

void MediaMetadataService::getCompleteMetadataFromTmdbId(int tmdbId, const QString& type)
{
    qDebug() << "[MediaMetadataService] getCompleteMetadataFromTmdbId called - tmdbId:" << tmdbId << "type:" << type;
    
    // Convert TMDB ID to contentId format and use the main method
    QString contentId = QString("tmdb:%1").arg(tmdbId);
    getCompleteMetadata(contentId, type);
}

void MediaMetadataService::onTmdbIdFound(const QString& imdbId, int tmdbId)
{
    qDebug() << "[MediaMetadataService] TMDB ID found - IMDB:" << imdbId << "TMDB:" << tmdbId;
    
    if (!m_pendingDetailsByImdbId.contains(imdbId)) {
        qWarning() << "[MediaMetadataService] Received TMDB ID for unknown IMDB ID:" << imdbId;
        return;
    }
    
    PendingRequest& request = m_pendingDetailsByImdbId[imdbId];
    request.tmdbId = tmdbId;
    m_tmdbIdToImdbId[tmdbId] = imdbId;
    
    if (!m_tmdbService) {
        qWarning() << "[MediaMetadataService] TMDB service not available, cannot fetch metadata";
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
    qDebug() << "[MediaMetadataService] TMDB movie metadata fetched for TMDB ID:" << tmdbId;
    
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
        emit error("Failed to convert TMDB movie data to detail map");
        return;
    }
    
    // Fetch OMDB ratings if we have an IMDB ID and API key is configured
    Configuration& config = Configuration::instance();
    if (!imdbId.isEmpty() && imdbId.startsWith("tt") && !config.omdbApiKey().isEmpty()) {
        if (!m_omdbService) {
            qWarning() << "[MediaMetadataService] OMDB service not available, emitting metadata without ratings";
            // Cache and emit metadata without OMDB ratings
            QString cacheKey = contentId + "|" + type;
            CachedMetadata cached;
            cached.data = details;
            cached.timestamp = QDateTime::currentDateTime();
            m_metadataCache[cacheKey] = cached;
            emit metadataLoaded(details);
            return;
        }
        // Store details keyed by IMDB ID for matching when OMDB response arrives
        m_pendingDetailsByImdbId[imdbId] = {contentId, type, tmdbId, details};
        qDebug() << "[MediaMetadataService] Stored pending details for IMDB ID:" << imdbId << "contentId:" << contentId;
        qDebug() << "[MediaMetadataService] Fetching OMDB ratings for IMDB ID:" << imdbId;
        m_omdbService->getRatings(imdbId);
    } else {
        // No IMDB ID or no API key, emit details without OMDB ratings
        if (imdbId.isEmpty() || !imdbId.startsWith("tt")) {
            qDebug() << "[MediaMetadataService] No IMDB ID available, skipping OMDB ratings";
        } else {
            qDebug() << "[MediaMetadataService] OMDB API key not configured, skipping ratings fetch";
        }
        
        // Cache and emit metadata
        QString cacheKey = contentId + "|" + type;
        CachedMetadata cached;
        cached.data = details;
        cached.timestamp = QDateTime::currentDateTime();
        m_metadataCache[cacheKey] = cached;
        qDebug() << "[MediaMetadataService] Cached metadata for:" << cacheKey;
        
        emit metadataLoaded(details);
    }
}

void MediaMetadataService::onTmdbTvMetadataFetched(int tmdbId, const QJsonObject& data)
{
    qDebug() << "[MediaMetadataService] TMDB TV metadata fetched for TMDB ID:" << tmdbId;
    
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
        emit error("Failed to convert TMDB TV data to detail map");
        return;
    }
    
    // Fetch OMDB ratings if we have an IMDB ID and API key is configured
    Configuration& config = Configuration::instance();
    if (!imdbId.isEmpty() && imdbId.startsWith("tt") && !config.omdbApiKey().isEmpty()) {
        if (!m_omdbService) {
            qWarning() << "[MediaMetadataService] OMDB service not available, emitting metadata without ratings";
            emit metadataLoaded(details);
            return;
        }
        // Store details keyed by IMDB ID for matching when OMDB response arrives
        m_pendingDetailsByImdbId[imdbId] = {contentId, type, tmdbId, details};
        qDebug() << "[MediaMetadataService] Stored pending details for IMDB ID:" << imdbId << "contentId:" << contentId;
        qDebug() << "[MediaMetadataService] Fetching OMDB ratings for IMDB ID:" << imdbId;
        m_omdbService->getRatings(imdbId);
    } else {
        // No IMDB ID or no API key, emit details without OMDB ratings
        if (imdbId.isEmpty() || !imdbId.startsWith("tt")) {
            qDebug() << "[MediaMetadataService] No IMDB ID available, skipping OMDB ratings";
        } else {
            qDebug() << "[MediaMetadataService] OMDB API key not configured, skipping ratings fetch";
        }
        emit metadataLoaded(details);
    }
}

void MediaMetadataService::onOmdbRatingsFetched(const QString& imdbId, const QJsonObject& data)
{
    qDebug() << "[MediaMetadataService] OMDB ratings fetched for IMDB ID:" << imdbId;
    qDebug() << "[MediaMetadataService] Pending details map keys:" << m_pendingDetailsByImdbId.keys();
    
    if (!m_pendingDetailsByImdbId.contains(imdbId)) {
        qWarning() << "[MediaMetadataService] OMDB ratings received but pending details map is empty for IMDB ID:" << imdbId;
        qWarning() << "[MediaMetadataService] Available keys in map:" << m_pendingDetailsByImdbId.keys();
        // This shouldn't happen, but if it does, we can't merge ratings
        // The details were likely already emitted without OMDB ratings
        return;
    }
    
    PendingRequest& request = m_pendingDetailsByImdbId[imdbId];
    QVariantMap details = request.details;
    
    qDebug() << "[MediaMetadataService] Merging OMDB ratings into details for contentId:" << request.contentId;
    
    // Merge OMDB ratings into details
    FrontendDataMapper::mergeOmdbRatings(details, data);
    
    // Cache and emit complete metadata
    QString cacheKey = request.contentId + "|" + request.type;
    CachedMetadata cached;
    cached.data = details;
    cached.timestamp = QDateTime::currentDateTime();
    m_metadataCache[cacheKey] = cached;
    qDebug() << "[MediaMetadataService] Cached complete metadata for:" << cacheKey;
    
    emit metadataLoaded(details);
    
    // Clean up
    m_pendingDetailsByImdbId.remove(imdbId);
}

void MediaMetadataService::onOmdbError(const QString& message, const QString& imdbId)
{
    qDebug() << "[MediaMetadataService] OMDB error for IMDB ID:" << imdbId << ":" << message;
    
    // Even if OMDB fails, emit the details we have from TMDB
    if (m_pendingDetailsByImdbId.contains(imdbId)) {
        PendingRequest& request = m_pendingDetailsByImdbId[imdbId];
        
        // Cache and emit metadata
        QString cacheKey = request.contentId + "|" + request.type;
        CachedMetadata cached;
        cached.data = request.details;
        cached.timestamp = QDateTime::currentDateTime();
        m_metadataCache[cacheKey] = cached;
        qDebug() << "[MediaMetadataService] Cached metadata (OMDB error) for:" << cacheKey;
        
        emit metadataLoaded(request.details);
        m_pendingDetailsByImdbId.remove(imdbId);
    }
}

void MediaMetadataService::onTmdbError(const QString& message)
{
    qWarning() << "[MediaMetadataService] TMDB error:" << message;
    emit error(message);
}

void MediaMetadataService::clearMetadataCache()
{
    m_metadataCache.clear();
    qDebug() << "[MediaMetadataService] Metadata cache cleared";
}

int MediaMetadataService::getMetadataCacheSize() const
{
    return m_metadataCache.size();
}

QVariantList MediaMetadataService::getSeriesEpisodes(const QString& contentId, int seasonNumber)
{
    qDebug() << "[MediaMetadataService] getSeriesEpisodes called - contentId:" << contentId << "season:" << seasonNumber;
    
    if (!m_seriesEpisodes.contains(contentId)) {
        qDebug() << "[MediaMetadataService] No episodes cached for series:" << contentId;
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
    
    qDebug() << "[MediaMetadataService] Found" << seasonEpisodes.size() << "episodes for season" << seasonNumber;
    return seasonEpisodes;
}

QVariantList MediaMetadataService::getSeriesEpisodesByTmdbId(int tmdbId, int seasonNumber)
{
    qDebug() << "[MediaMetadataService] getSeriesEpisodesByTmdbId called - tmdbId:" << tmdbId << "season:" << seasonNumber;
    
    // Look up contentId from TMDB ID
    if (!m_tmdbIdToContentId.contains(tmdbId)) {
        qDebug() << "[MediaMetadataService] No contentId mapping found for TMDB ID:" << tmdbId;
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
            qDebug() << "[MediaMetadataService] Found AIOMetadata addon:" << addon.id;
            return addon;
        }
    }
    
    qDebug() << "[MediaMetadataService] AIOMetadata addon not found";
    return AddonConfig();
}

void MediaMetadataService::fetchMetadataFromAddon(const QString& contentId, const QString& type)
{
    AddonConfig addon = findAiometadataAddon();
    if (addon.id.isEmpty()) {
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
    
    qDebug() << "[MediaMetadataService] Fetching metadata from addon - type:" << stremioType << "id:" << contentId;
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
        qWarning() << "[MediaMetadataService] Received meta for unknown request";
        client->deleteLater();
        return;
    }
    
    QString cacheKey = m_pendingAddonRequests.take(client);
    
    if (!m_pendingDetailsByImdbId.contains(cacheKey)) {
        qWarning() << "[MediaMetadataService] No pending request found for cache key:" << cacheKey;
        client->deleteLater();
        return;
    }
    
    PendingRequest& request = m_pendingDetailsByImdbId[cacheKey];
    
    // Normalize type back (Stremio uses "series", we use "tv")
    QString normalizedType = (type == "series") ? "tv" : type;
    
    qDebug() << "[MediaMetadataService] Addon metadata fetched - type:" << normalizedType << "contentId:" << request.contentId;
    
    // Extract the "meta" object from the response
    // AIOMetadata wraps metadata in a "meta" key: {"meta": {...}}
    QJsonObject meta;
    if (response.contains("meta") && response["meta"].isObject()) {
        meta = response["meta"].toObject();
        qDebug() << "[MediaMetadataService] Extracted meta object from response";
    } else {
        // Fallback: use the response directly if it doesn't have a "meta" wrapper
        qDebug() << "[MediaMetadataService] No 'meta' wrapper found, using response directly";
        meta = response;
    }
    
    // Map Stremio metadata format to our detail format
    QVariantMap details = FrontendDataMapper::mapAddonMetaToDetailVariantMap(meta, request.contentId, normalizedType);
    
    if (details.isEmpty()) {
        emit error("Failed to convert addon metadata to detail map");
        m_pendingDetailsByImdbId.remove(cacheKey);
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
        qDebug() << "[MediaMetadataService] Extracted" << episodes.size() << "episodes for series" << request.contentId;
        
        // Store mapping from TMDB ID to contentId for episode lookups
        QString tmdbIdStr = details["tmdbId"].toString();
        if (!tmdbIdStr.isEmpty()) {
            bool ok;
            int tmdbId = tmdbIdStr.toInt(&ok);
            if (ok && tmdbId > 0) {
                m_tmdbIdToContentId[tmdbId] = request.contentId;
                qDebug() << "[MediaMetadataService] Mapped TMDB ID" << tmdbId << "to contentId" << request.contentId;
            }
        }
    }
    
    // Cache and emit metadata
    CachedMetadata cached;
    cached.data = details;
    cached.timestamp = QDateTime::currentDateTime();
    m_metadataCache[cacheKey] = cached;
    qDebug() << "[MediaMetadataService] Cached metadata from addon for:" << cacheKey;
    
    emit metadataLoaded(details);
    
    // Clean up
    m_pendingDetailsByImdbId.remove(cacheKey);
    client->deleteLater();
}

void MediaMetadataService::onAddonMetaError(const QString& errorMessage)
{
    AddonClient* client = qobject_cast<AddonClient*>(sender());
    if (!client) {
        qWarning() << "[MediaMetadataService] Received error from unknown client";
        return;
    }
    
    if (!m_pendingAddonRequests.contains(client)) {
        qWarning() << "[MediaMetadataService] Received error for unknown request";
        client->deleteLater();
        return;
    }
    
    QString cacheKey = m_pendingAddonRequests.take(client);
    m_pendingDetailsByImdbId.remove(cacheKey);
    
    qWarning() << "[MediaMetadataService] Addon metadata error:" << errorMessage;
    emit error(QString("Failed to fetch metadata from AIOMetadata: %1").arg(errorMessage));
    
    client->deleteLater();
}


