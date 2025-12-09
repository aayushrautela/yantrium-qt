#include "stream_service.h"
#include "logging_service.h"
#include "features/addons/logic/addon_repository.h"
#include "features/addons/logic/addon_client.h"
#include "features/addons/models/addon_config.h"
#include "features/addons/models/addon_manifest.h"
#include "core/services/library_service.h"
#include "core/services/id_parser.h"
#include "core/services/torrent_service.h"
#include "core/models/stream_info.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QList>

StreamService::StreamService(
    std::shared_ptr<AddonRepository> addonRepository,
    LibraryService* libraryService,
    QObject* parent)
    : QObject(parent)
    , m_addonRepository(std::move(addonRepository))
    , m_libraryService(libraryService)
    , m_torrentService(nullptr)
    , m_completedRequests(0)
    , m_totalRequests(0)
{
    // Initialize torrent service if available
    m_torrentService = new TorrentService(this);
    if (!m_torrentService->isAvailable()) {
        LoggingService::logInfo("StreamService", "Torrent support not available");
    }
}

QString StreamService::extractImdbId(const QVariantMap& itemData)
{
    // Check if already an IMDB ID
    QString id = itemData["id"].toString();
    if (IdParser::isImdbId(id)) {
        LoggingService::logDebug("StreamService", QString("Item already has IMDB ID: %1").arg(id));
        return id;
    }
    
    // Try to extract from imdbId field
    QString imdbId = itemData["imdbId"].toString();
    if (!imdbId.isEmpty()) {
        LoggingService::logDebug("StreamService", QString("Found IMDB ID in itemData: %1").arg(imdbId));
        return imdbId;
    }
    
    // If we can't find IMDB ID, return empty (addons should provide IMDB IDs)
    LoggingService::logWarning("StreamService", "Could not extract IMDB ID from itemData");
    return QString();
}

QString StreamService::formatEpisodeId(int season, int episode)
{
    return QString("S%1E%2").arg(season, 2, 10, QChar('0')).arg(episode, 2, 10, QChar('0'));
}

void StreamService::getStreamsForItem(const QVariantMap& itemData, const QString& episodeId)
{
    LoggingService::logDebug("StreamService", QString("Getting streams for item: %1 type: %2").arg(itemData["name"].toString(), itemData["type"].toString()));
    
    m_currentItemData = itemData;
    m_currentEpisodeId = episodeId;
    m_allStreams.clear();
    m_pendingRequests.clear();
    m_completedRequests = 0;
    m_totalRequests = 0;
    
    if (!m_addonRepository) {
        LoggingService::report("Addon repository not initialized", "SERVICE_UNAVAILABLE", "StreamService");
        emit error("Addon repository not initialized");
        return;
    }
    
    // Get IMDB ID
    QString imdbId = extractImdbId(itemData);
    if (imdbId.isEmpty()) {
        LoggingService::report("Could not get IMDB ID for item", "ID_EXTRACTION_ERROR", "StreamService");
        emit error("Could not get IMDB ID for item");
        return;
    }
    
    m_currentImdbId = imdbId;
    fetchStreamsFromAddons();
}

void StreamService::fetchStreamsFromAddons()
{
    if (!m_addonRepository) {
        LoggingService::report("Addon repository not initialized", "SERVICE_UNAVAILABLE", "StreamService");
        emit error("Addon repository not initialized");
        return;
    }
    
    QString type = m_currentItemData["type"].toString();
    
    // Get enabled addons
    QList<AddonConfig> enabledAddons = m_addonRepository->getEnabledAddons();
    qDebug() << "[StreamService] Found" << enabledAddons.size() << "enabled addon(s)";
    
    // Filter addons that support streaming and the item type
    QList<AddonConfig> streamingAddons;
    for (const AddonConfig& addon : enabledAddons) {
        AddonManifest manifest = m_addonRepository->getManifest(addon);
        if (manifest.id().isEmpty()) {
            continue;
        }
        
        QJsonArray resources = manifest.resources();
        bool hasStreamResource = AddonRepository::hasResource(resources, "stream");
        bool supportsType = manifest.types().contains(type);
        
        qDebug() << "[StreamService] Addon" << addon.name << "(" << addon.id << "): hasStream=" << hasStreamResource << ", supportsType=" << supportsType;
        
        if (hasStreamResource && supportsType) {
            streamingAddons.append(addon);
        }
    }
    
    qDebug() << "[StreamService]" << streamingAddons.size() << "addon(s) support streaming for" << type;
    
    if (streamingAddons.isEmpty()) {
        emit streamsLoaded(QVariantList());
        return;
    }
    
    // Determine stream ID (episode ID for TV, IMDB ID otherwise)
    QString streamId = m_currentEpisodeId.isEmpty() ? m_currentImdbId : m_currentEpisodeId;
    qDebug() << "[StreamService] Using stream ID:" << streamId;
    
    // Create AddonClient for each addon and fetch streams
    m_totalRequests = streamingAddons.size();
    m_completedRequests = 0;
    
    for (const AddonConfig& addon : streamingAddons) {
        QString baseUrl = AddonClient::extractBaseUrl(addon.manifestUrl);
        AddonClient* client = new AddonClient(baseUrl, this);
        
        PendingRequest request;
        request.addonId = addon.id;
        request.addonName = addon.name;
        request.type = type;
        request.streamId = streamId;
        m_pendingRequests.append(request);
        
        connect(client, &AddonClient::streamsFetched, this, &StreamService::onStreamsFetched);
        connect(client, &AddonClient::error, this, &StreamService::onAddonError);
        
        qDebug() << "[StreamService] Requesting streams from addon" << addon.name << "for" << type << streamId;
        client->getStreams(type, streamId);
    }
}

void StreamService::onStreamsFetched(const QString& type, const QString& id, const QJsonArray& streams)
{
    Q_UNUSED(type);
    Q_UNUSED(id);
    
    // Find which addon this response is from
    // We can't directly identify from the signal, so we'll use the first pending request
    // In a more robust implementation, we'd track client->request mappings
    if (m_pendingRequests.isEmpty()) {
        qWarning() << "[StreamService] Received streams but no pending requests";
        m_completedRequests++;
        checkAllRequestsComplete();
        return;
    }
    
    PendingRequest request = m_pendingRequests.takeFirst();
    qDebug() << "[StreamService] Received" << streams.size() << "stream(s) from addon" << request.addonName;
    
    processStreamsFromAddon(request.addonId, request.addonName, streams);
    
    m_completedRequests++;
    checkAllRequestsComplete();
}

void StreamService::processStreamsFromAddon(const QString& addonId, const QString& addonName, const QJsonArray& streams)
{
    for (const QJsonValue& value : streams) {
        if (!value.isObject()) {
            continue;
        }
        
        QJsonObject streamObj = value.toObject();
        
        // Pre-filter: must have playable link and identifier
        bool hasPlayableLink = streamObj.contains("url") || streamObj.contains("infoHash");
        bool hasIdentifier = streamObj.contains("title") || streamObj.contains("name");
        
        if (!hasPlayableLink || !hasIdentifier) {
            qDebug() << "[StreamService] Skipping stream without playable link or identifier";
            continue;
        }
        
        try {
            // Extract URL
            QString streamUrl = StreamInfo::extractStreamUrl(streamObj);
            if (streamUrl.isEmpty()) {
                qDebug() << "[StreamService] Could not extract stream URL";
                continue;
            }
            
            // Check if it's a magnet link
            if (m_torrentService && m_torrentService->isMagnetLink(streamUrl)) {
                if (!m_torrentService->isAvailable()) {
                    // Torrent support not available - skip this stream
                    LoggingService::logWarning("StreamService", 
                        "Skipping magnet link stream - torrent support not available. Install libtorrent-rasterbar to enable torrent streaming.");
                    continue;
                }
                
                // Convert magnet link to streamable URL
                int fileIndex = -1;
                if (streamObj.contains("fileIdx") && streamObj["fileIdx"].isDouble()) {
                    fileIndex = streamObj["fileIdx"].toInt();
                }
                
                QString convertedUrl = convertMagnetToStreamUrl(streamUrl, fileIndex);
                if (!convertedUrl.isEmpty()) {
                    streamUrl = convertedUrl;
                    LoggingService::logInfo("StreamService", 
                        QString("Converted magnet link to streamable URL: %1").arg(streamUrl));
                } else {
                    LoggingService::logWarning("StreamService", 
                        "Failed to convert magnet link to streamable URL");
                    continue; // Skip this stream if conversion failed
                }
            }
            
            // Update streamObj with extracted URL if it was missing
            if (!streamObj.contains("url") || streamObj["url"].isNull()) {
                streamObj["url"] = streamUrl;
            }
            
            // Determine display title (prefer description if longer and contains newlines)
            QString displayTitle = streamObj["title"].toString();
            if (displayTitle.isEmpty()) {
                displayTitle = streamObj["name"].toString();
            }
            if (displayTitle.isEmpty()) {
                displayTitle = "Unnamed Stream";
            }
            
            QString description = streamObj["description"].toString();
            QString title = streamObj["title"].toString();
            if (!description.isEmpty() && description.contains('\n') && 
                description.length() > title.length()) {
                displayTitle = description;
            }
            
            // Extract size from behaviorHints or top-level
            int sizeInBytes = -1;
            if (streamObj.contains("behaviorHints") && streamObj["behaviorHints"].isObject()) {
                QJsonObject behaviorHints = streamObj["behaviorHints"].toObject();
                if (behaviorHints.contains("videoSize")) {
                    sizeInBytes = behaviorHints["videoSize"].toInt(-1);
                }
            }
            if (sizeInBytes < 0 && streamObj.contains("size")) {
                sizeInBytes = streamObj["size"].toInt(-1);
            }
            
            // Create StreamInfo
            StreamInfo info = StreamInfo::fromJson(streamObj, addonId, addonName);
            info.title = displayTitle;
            if (sizeInBytes >= 0) {
                info.size = sizeInBytes;
            }
            
            // Convert to QVariantMap for QML
            QVariantMap streamMap = info.toVariantMap();
            m_allStreams.append(streamMap);
            
        } catch (const std::exception& e) {
            qWarning() << "[StreamService] Failed to process stream:" << e.what();
        }
    }
}

void StreamService::checkAllRequestsComplete()
{
    if (m_completedRequests >= m_totalRequests) {
        qDebug() << "[StreamService] All requests complete. Total streams:" << m_allStreams.size();
        emit streamsLoaded(m_allStreams);
    }
}

void StreamService::onAddonError(const QString& errorMessage)
{
    qWarning() << "[StreamService] Addon error:" << errorMessage;
    
    // Still count as completed
    if (!m_pendingRequests.isEmpty()) {
        m_pendingRequests.removeFirst();
    }
    m_completedRequests++;
    checkAllRequestsComplete();
}

QString StreamService::convertMagnetToStreamUrl(const QString& url, int fileIndex)
{
    if (!m_torrentService || !m_torrentService->isAvailable()) {
        return QString();
    }
    
    return m_torrentService->getStreamUrl(url, fileIndex);
}


