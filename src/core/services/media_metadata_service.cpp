#include "media_metadata_service.h"
#include "tmdb_data_service.h"
#include "omdb_service.h"
#include "trakt_core_service.h"
#include "frontend_data_mapper.h"
#include "id_parser.h"
#include "configuration.h"
#include <QDebug>
#include <QJsonObject>
#include <QDateTime>

MediaMetadataService::MediaMetadataService(
    std::shared_ptr<TmdbDataService> tmdbService,
    std::shared_ptr<OmdbService> omdbService,
    TraktCoreService* traktService,
    QObject* parent)
    : QObject(parent)
    , m_tmdbService(std::move(tmdbService))
    , m_omdbService(std::move(omdbService))
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
    
    // Try to extract TMDB ID
    int tmdbId = IdParser::extractTmdbId(contentId);
    
    // If IMDB ID, search for TMDB ID first
    if (tmdbId == 0 && contentId.startsWith("tt")) {
        if (!m_tmdbService) {
            qWarning() << "[MediaMetadataService] TMDB service not available, cannot fetch metadata";
            emit error("TMDB service not available");
            return;
        }
        qDebug() << "[MediaMetadataService] IMDB ID detected, fetching TMDB ID first";
        m_tmdbService->getTmdbIdFromImdb(contentId);
        
        // Store pending request
        PendingRequest request;
        request.contentId = contentId;
        request.type = type;
        request.tmdbId = 0;
        m_pendingDetailsByImdbId[contentId] = request;
        return;
    }
    
    if (tmdbId == 0) {
        emit error("Could not extract TMDB ID from content ID");
        return;
    }
    
    // Store TMDB ID mapping for direct TMDB IDs
    // Store both the original contentId and type in a pending request
    // We'll use the IMDB ID as the key once we extract it from TMDB response
    PendingRequest request;
    request.contentId = contentId;
    request.type = type;
    request.tmdbId = tmdbId;
    // Store temporarily with TMDB ID as string key until we get IMDB ID
    m_pendingDetailsByImdbId[QString("tmdb:%1").arg(tmdbId)] = request;
    m_tmdbIdToImdbId[tmdbId] = contentId; // Keep this for backward compatibility
    
    // Fetch metadata based on type
    getCompleteMetadataFromTmdbId(tmdbId, type);
}

void MediaMetadataService::getCompleteMetadataFromTmdbId(int tmdbId, const QString& type)
{
    qDebug() << "[MediaMetadataService] getCompleteMetadataFromTmdbId called - tmdbId:" << tmdbId << "type:" << type;
    
    if (!m_tmdbService) {
        qWarning() << "[MediaMetadataService] TMDB service not available, cannot fetch metadata";
        emit error("TMDB service not available");
        return;
    }
    
    // Fetch metadata based on type
    if (type == "movie") {
        m_tmdbService->getMovieMetadata(tmdbId);
    } else if (type == "series" || type == "tv") {
        m_tmdbService->getTvMetadata(tmdbId);
    } else {
        emit error(QString("Unknown content type: %1").arg(type));
    }
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


