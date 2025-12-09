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
#include <QDate>
#include <QRegularExpression>
#include <QStringList>
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
        QVariantMap cachedDetails = cached.toMap();
        
        // Even if metadata is cached, check if we have episodes stored
        // Episodes might not have been extracted if this was cached before episode extraction was implemented
        if ((type == "tv" || type == "series") && !m_seriesEpisodes.contains(contentId)) {
            LoggingService::logDebug("MediaMetadataService", QString("Metadata cached but no episodes found, will try to extract from cached data"));
            // Try to extract episodes from cached metadata if available
            // This is a fallback - normally episodes should be extracted during initial fetch
        }
        
        emit metadataLoaded(cachedDetails);
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
    LoggingService::logDebug("MediaMetadataService", QString("getSeriesEpisodes called: contentId=%1, season=%2").arg(contentId).arg(seasonNumber));
    LoggingService::logDebug("MediaMetadataService", QString("Available episode keys: %1").arg(QStringList(m_seriesEpisodes.keys()).join(", ")));
    
    if (!m_seriesEpisodes.contains(contentId)) {
        LoggingService::logWarning("MediaMetadataService", QString("No episodes found for contentId: %1").arg(contentId));
        return QVariantList();
    }
    
    QVariantList allEpisodes = m_seriesEpisodes[contentId];
    LoggingService::logDebug("MediaMetadataService", QString("Found %1 total episodes for contentId: %2").arg(allEpisodes.size()).arg(contentId));
    
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
        LoggingService::logDebug("MediaMetadataService", QString("Checking for episodes in metadata - has videos: %1").arg(meta.contains("videos")));
        
        // Extract show runtime as fallback for episodes that don't have individual runtime
        int showRuntime = 0;
        if (meta.contains("runtime")) {
            QJsonValue runtimeValue = meta["runtime"];
            if (runtimeValue.isDouble()) {
                showRuntime = runtimeValue.toInt();
            } else if (runtimeValue.isString()) {
                // Parse runtime string like "49 min"
                QString runtimeStr = runtimeValue.toString();
                QRegularExpression rx("(\\d+)");
                QRegularExpressionMatch match = rx.match(runtimeStr);
                if (match.hasMatch()) {
                    showRuntime = match.captured(1).toInt();
                }
            }
        }
        
        if (meta.contains("videos") && meta["videos"].isArray()) {
            QJsonArray videosArray = meta["videos"].toArray();
            LoggingService::logDebug("MediaMetadataService", QString("Found videos array with %1 items").arg(videosArray.size()));
            for (const QJsonValue& value : videosArray) {
                if (value.isObject()) {
                    QJsonObject videoObj = value.toObject();
                    // Check if it's an episode (has season and episode numbers)
                    // Episodes can have "episode" or "number" field for episode number
                    bool hasEpisodeNumber = videoObj.contains("episode") || videoObj.contains("number");
                    if (videoObj.contains("season") && hasEpisodeNumber) {
                        QVariantMap episode;
                        episode["season"] = videoObj["season"].toInt();
                        
                        // Get episode number - prefer "episode", fallback to "number"
                        int episodeNum = 0;
                        if (videoObj.contains("episode")) {
                            episodeNum = videoObj["episode"].toInt();
                        } else if (videoObj.contains("number")) {
                            episodeNum = videoObj["number"].toInt();
                        }
                        episode["episodeNumber"] = episodeNum;
                        
                        // Get title - prefer "name", fallback to "title"
                        QString title = videoObj["name"].toString();
                        if (title.isEmpty()) {
                            title = videoObj["title"].toString();
                        }
                        episode["title"] = title;
                        
                        // Get description - prefer "overview", fallback to "description"
                        QString description = videoObj["overview"].toString();
                        if (description.isEmpty()) {
                            description = videoObj["description"].toString();
                        }
                        episode["description"] = description;
                        
                        // Get air date - prefer "released", fallback to "firstAired"
                        QString airDate = videoObj["released"].toString();
                        if (airDate.isEmpty()) {
                            airDate = videoObj["firstAired"].toString();
                        }
                        episode["airDate"] = airDate;
                        
                        // Get thumbnail
                        episode["thumbnailUrl"] = videoObj["thumbnail"].toString();
                        
                        // Extract duration if available (from runtime field, may be string like "49 min")
                        // Fallback to show runtime if episode doesn't have individual runtime
                        int duration = 0;
                        if (videoObj.contains("runtime")) {
                            QJsonValue runtimeValue = videoObj["runtime"];
                            if (runtimeValue.isDouble()) {
                                duration = runtimeValue.toInt();
                            } else if (runtimeValue.isString()) {
                                // Try to extract number from string like "49 min"
                                QString runtimeStr = runtimeValue.toString();
                                // Simple regex to extract first number
                                QRegularExpression rx("(\\d+)");
                                QRegularExpressionMatch match = rx.match(runtimeStr);
                                if (match.hasMatch()) {
                                    duration = match.captured(1).toInt();
                                }
                            }
                        }
                        // Use show runtime as fallback if episode doesn't have runtime
                        if (duration == 0 && showRuntime > 0) {
                            duration = showRuntime;
                        }
                        episode["duration"] = duration;
                        
                        // Format metadata line: "Release Date • Runtime" (e.g., "Jan 21, 2008 • 49m")
                        QString metadataLine = "";
                        if (!airDate.isEmpty()) {
                            LoggingService::logDebug("MediaMetadataService", "Parsing airDate: " + airDate);
                            // Extract just the date part (YYYY-MM-DD) from the ISO string
                            QString datePart = airDate.left(10); // "YYYY-MM-DD"
                            
                            // Parse the date part
                            QDate date = QDate::fromString(datePart, "yyyy-MM-dd");
                            
                            if (date.isValid()) {
                                // Format as "Jan 21, 2008"
                                metadataLine = date.toString("MMM d, yyyy");
                                LoggingService::logDebug("MediaMetadataService", "Formatted date: " + metadataLine);
                            } else {
                                // Fallback to just showing the date part if parsing fails for some reason
                                metadataLine = datePart;
                                LoggingService::logWarning("MediaMetadataService", "Date parsing failed for: " + datePart + ". Using fallback.");
                            }
                        }
                        
                        // Add runtime if available
                        if (duration > 0) {
                            if (!metadataLine.isEmpty()) {
                                metadataLine += " • " + QString::number(duration) + "m";
                            } else {
                                metadataLine = QString::number(duration) + "m";
                            }
                        }
                        
                        episode["metadataLine"] = metadataLine;
                        
                        // Store additional fields that might be useful
                        if (videoObj.contains("rating")) {
                            episode["rating"] = videoObj["rating"].toString();
                        }
                        if (videoObj.contains("id")) {
                            episode["id"] = videoObj["id"].toString();
                        }
                        if (videoObj.contains("tvdb_id")) {
                            episode["tvdbId"] = videoObj["tvdb_id"].toInt();
                        }
                        
                        episodes.append(episode);
                        LoggingService::logDebug("MediaMetadataService", QString("Extracted episode S%1E%2: %3")
                            .arg(episode["season"].toInt()).arg(episode["episodeNumber"].toInt()).arg(episode["title"].toString()));
                    } else {
                        LoggingService::logDebug("MediaMetadataService", "Skipping video - missing season or episode number");
                    }
                }
            }
        } else {
            LoggingService::logWarning("MediaMetadataService", "No videos array found in metadata for series");
        }
        // Store episodes for this series under multiple ID formats for easier lookup
        // Store under the requested contentId
        m_seriesEpisodes[request.contentId] = episodes;
        
        // Also store under IMDB ID if available (from the metadata)
        // Try to get from meta object first, then from details
        QString imdbId;
        if (meta.contains("imdb_id") && meta["imdb_id"].isString()) {
            imdbId = meta["imdb_id"].toString();
        } else if (meta.contains("id") && meta["id"].isString() && meta["id"].toString().startsWith("tt")) {
            imdbId = meta["id"].toString();
        } else {
            imdbId = details["imdbId"].toString();
        }
        
        if (!imdbId.isEmpty() && imdbId != request.contentId) {
            m_seriesEpisodes[imdbId] = episodes;
            LoggingService::logDebug("MediaMetadataService", QString("Also stored episodes under IMDB ID: %1").arg(imdbId));
        }
        
        // Also store under TMDB ID format if we have TMDB ID
        QString tmdbId = details["tmdbId"].toString();
        if (tmdbId.isEmpty() && meta.contains("tmdb_id")) {
            tmdbId = meta["tmdb_id"].toString();
        }
        if (!tmdbId.isEmpty()) {
            QString tmdbKey = "tmdb:" + tmdbId;
            if (tmdbKey != request.contentId && tmdbKey != imdbId) {
                m_seriesEpisodes[tmdbKey] = episodes;
                LoggingService::logDebug("MediaMetadataService", QString("Also stored episodes under TMDB key: %1").arg(tmdbKey));
            }
        }
        
        LoggingService::logDebug("MediaMetadataService", QString("Extracted %1 episodes for series %2").arg(episodes.size()).arg(request.contentId));
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

