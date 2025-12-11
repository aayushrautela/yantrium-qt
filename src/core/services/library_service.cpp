#include "library_service.h"
#include "logging_service.h"
#include "logging_service.h"
#include "core/database/database_manager.h"
#include "core/services/id_parser.h"
#include "core/services/configuration.h"
#include "core/services/frontend_data_mapper.h"
#include "core/services/local_library_service.h"
#include "features/addons/logic/addon_client.h"
#include "features/addons/models/addon_config.h"
#include "features/addons/models/addon_manifest.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <algorithm>

LibraryService::LibraryService(
    std::shared_ptr<AddonRepository> addonRepository,
    std::shared_ptr<MediaMetadataService> mediaMetadataService,
    std::shared_ptr<OmdbService> omdbService,
    std::shared_ptr<LocalLibraryService> localLibraryService,
    std::unique_ptr<CatalogPreferencesDao> catalogPreferencesDao,
    TraktCoreService* traktService,
    QObject* parent)
    : QObject(parent)
    , m_addonRepository(std::move(addonRepository))
    , m_traktService(traktService)
    , m_mediaMetadataService(std::move(mediaMetadataService))
    , m_omdbService(std::move(omdbService))
    , m_localLibraryService(std::move(localLibraryService))
    , m_catalogPreferencesDao(std::move(catalogPreferencesDao))
    , m_pendingCatalogRequests(0)
    , m_isLoadingCatalogs(false)
    , m_isRawExport(false)
    , m_pendingHeroRequests(0)
    , m_isLoadingHeroItems(false)
    , m_pendingContinueWatchingMetadataRequests(0)
    , m_pendingSearchRequests(0)
{
    // Connect to Trakt service for continue watching
    if (m_traktService) {
        connect(m_traktService, &TraktCoreService::playbackProgressFetched,
                this, &LibraryService::onPlaybackProgressFetched);
    }
    
    // Connect to MediaMetadataService for item details
    if (m_mediaMetadataService) {
        connect(m_mediaMetadataService.get(), &MediaMetadataService::metadataLoaded,
                this, &LibraryService::onMediaMetadataLoaded);
        connect(m_mediaMetadataService.get(), &MediaMetadataService::error,
                this, &LibraryService::onMediaMetadataError);
    }

    // Connect to LocalLibraryService for smart play
    if (m_localLibraryService) {
        connect(m_localLibraryService.get(), &LocalLibraryService::watchProgressLoaded,
                this, &LibraryService::onWatchProgressLoaded);
    }
}

void LibraryService::loadCatalogs()
{
    loadCatalogsRaw(false);
}

void LibraryService::loadCatalogsRaw()
{
    loadCatalogsRaw(true);
}

void LibraryService::loadCatalogsRaw(bool rawMode)
{
    // Removed verbose debug logs - unnecessary
    
    if (m_isLoadingCatalogs) {
        return;
    }
    
    m_isLoadingCatalogs = true;
    m_isRawExport = rawMode;
    m_catalogSections.clear();
    m_rawCatalogData.clear();
    m_pendingCatalogRequests = 0;
    
    // Clean up old clients
    qDeleteAll(m_activeClients);
    m_activeClients.clear();
    
    // Get enabled addons
    QList<AddonConfig> enabledAddons = m_addonRepository->getEnabledAddons();
    
    if (enabledAddons.isEmpty()) {
        m_isLoadingCatalogs = false;
        emit catalogsLoaded(QVariantList());
        return;
    }
    
    // For each enabled addon, fetch its catalogs
    for (const AddonConfig& addon : enabledAddons) {
        AddonManifest manifest = m_addonRepository->getManifest(addon);
        if (manifest.id().isEmpty()) {
            continue;
        }
        
        // Get catalogs from manifest
        QList<CatalogDefinition> catalogs = manifest.catalogs();
        
        if (catalogs.isEmpty()) {
            continue;
        }
        
        // Get base URL for this addon
        QString baseUrl = AddonClient::extractBaseUrl(addon.manifestUrl);
        
        int enabledCatalogCount = 0;
        int disabledCatalogCount = 0;
        
        // Fetch each catalog (only if enabled)
        // IMPORTANT: Create a separate AddonClient for each catalog request
        // because AddonClient only supports one request at a time (single m_currentReply)
        for (const CatalogDefinition& catalog : catalogs) {
            QString catalogName = catalog.name().isEmpty() ? catalog.type() : catalog.name();
            QString catalogId = catalog.id();
            QString catalogType = catalog.type();
            
            // Removed debug log - unnecessary
            
            // Check if this catalog is enabled (default to enabled if no preference exists)
            auto preference = m_catalogPreferencesDao->getPreference(addon.id, catalogType, catalogId);
            bool isEnabled = preference ? preference->enabled : true;
            
            if (!isEnabled) {
                disabledCatalogCount++;
                continue;
            }
            
            enabledCatalogCount++;
            m_pendingCatalogRequests++;
            
            // Create a NEW AddonClient for each catalog request
            // This is necessary because AddonClient only supports one concurrent request
            AddonClient* client = new AddonClient(baseUrl, this);
            m_activeClients.append(client);
            
            // Store addon ID and catalog info on client for this specific request
            client->setProperty("addonId", addon.id);
            client->setProperty("baseUrl", baseUrl);
            client->setProperty("catalogType", catalogType);
            client->setProperty("catalogId", catalogId);
            client->setProperty("catalogName", catalogName);
            
            // Connect signals
            connect(client, &AddonClient::catalogFetched, this, &LibraryService::onCatalogFetched);
            connect(client, &AddonClient::error, this, &LibraryService::onClientError);
            
            client->getCatalog(catalogType, catalogId);
        }
    }
    
    // Also load continue watching
    m_traktService->getPlaybackProgressWithImages();
    
    // If no requests were made, finish immediately
    if (m_pendingCatalogRequests == 0) {
        finishLoadingCatalogs();
    }
}

void LibraryService::loadCatalog(const QString& addonId, const QString& type, const QString& id)
{
    AddonConfig addon = m_addonRepository->getAddon(addonId);
    if (addon.id.isEmpty()) {
        QString errorMsg = QString("Addon not found: %1").arg(addonId);
        LoggingService::report(errorMsg, "ADDON_NOT_FOUND", "LibraryService");
        emit error(errorMsg);
        return;
    }

    QString baseUrl = AddonClient::extractBaseUrl(addon.manifestUrl);
    AddonClient* client = new AddonClient(baseUrl, this);
    m_activeClients.append(client);
    
    connect(client, &AddonClient::catalogFetched, this, &LibraryService::onCatalogFetched);
    connect(client, &AddonClient::error, this, &LibraryService::onClientError);
    
    client->getCatalog(type, id);
}

void LibraryService::loadHeroItems()
{
    // Removed verbose debug logs - unnecessary
    
    if (m_isLoadingHeroItems) {
        return;
    }
    
    m_isLoadingHeroItems = true;
    m_heroItems.clear();
    m_pendingHeroRequests = 0;
    
    // Get hero catalogs from preferences
    QList<CatalogPreferenceRecord> heroCatalogs = m_catalogPreferencesDao->getHeroCatalogs();
    
    if (heroCatalogs.isEmpty()) {
        // Removed debug log - unnecessary
        m_isLoadingHeroItems = false;
        emit heroItemsLoaded(QVariantList());
        return;
    }
    
    int itemsPerCatalog = qMax(1, 10 / heroCatalogs.size()); // Distribute items across catalogs
    
    for (const CatalogPreferenceRecord& heroCatalog : heroCatalogs) {
        try {
            AddonConfig addon = m_addonRepository->getAddon(heroCatalog.addonId);
            if (addon.id.isEmpty()) {
                // Removed warning log - unnecessary
                continue;
            }

            QString baseUrl = AddonClient::extractBaseUrl(addon.manifestUrl);
            AddonClient* client = new AddonClient(baseUrl, this);
            m_activeClients.append(client);
            m_pendingHeroRequests++;
            
            // Store hero catalog info on client
            client->setProperty("isHeroRequest", true);
            client->setProperty("addonId", heroCatalog.addonId);
            client->setProperty("baseUrl", baseUrl);
            client->setProperty("itemsPerCatalog", itemsPerCatalog);
            
            // Connect to handle response
            connect(client, &AddonClient::catalogFetched, this, &LibraryService::onHeroCatalogFetched);
            connect(client, &AddonClient::error, this, &LibraryService::onHeroClientError);
            
            // Request catalog
            // Removed debug log - unnecessary
            client->getCatalog(heroCatalog.catalogType, heroCatalog.catalogId);
            
        } catch (...) {
            LoggingService::logWarning("LibraryService", QString("Exception loading hero catalog: %1").arg(heroCatalog.addonId));
            continue;
        }
    }
    
    // If no requests were made, emit empty
    if (m_pendingHeroRequests == 0) {
        m_isLoadingHeroItems = false;
        emit heroItemsLoaded(QVariantList());
    }
}

void LibraryService::searchCatalogs(const QString& query)
{
    if (query.trimmed().isEmpty()) {
        emit searchResultsLoaded(QVariantList());
        return;
    }
    
    LoggingService::logInfo("LibraryService", QString("Searching catalogs for: %1").arg(query));
    
    // Clear any existing search clients
    for (AddonClient* client : m_activeClients) {
        if (client->property("isSearchRequest").toBool()) {
            client->deleteLater();
        }
    }
    m_activeClients.removeAll(nullptr);
    
    m_pendingSearchRequests = 0;
    
    // Structure to hold search catalog info with ordering
    struct SearchCatalogInfo {
        AddonConfig addon;
        CatalogDefinition catalog;
        QString baseUrl;
        int order;
        QString catalogId;
        
        bool operator<(const SearchCatalogInfo& other) const {
            return order < other.order;
        }
    };
    
    QList<SearchCatalogInfo> searchCatalogs;
    
    // Get all enabled addons and collect searchable catalogs
    QList<AddonConfig> addons = m_addonRepository->getEnabledAddons();

    for (const AddonConfig& addon : addons) {
        QString baseUrl = AddonClient::extractBaseUrl(addon.manifestUrl);
        if (baseUrl.isEmpty()) {
            continue;
        }

        AddonManifest manifest = m_addonRepository->getManifest(addon);
        
        // Iterate through the addon's catalogs to find searchable ones
        for (const CatalogDefinition& catalog : manifest.catalogs()) {
            bool isSearchable = false;
            for (const QJsonObject& extra : catalog.extra()) {
                if (extra["name"].toString() == "search") {
                    isSearchable = true;
                    break;
                }
            }

            if (!isSearchable) {
                continue;
            }
            
            // Get the order from preferences (default to 0)
            QString catalogId = catalog.id().isEmpty() ? QString() : catalog.id();
            auto preference = m_catalogPreferencesDao->getPreference(addon.id, catalog.type(), catalogId);
            int order = preference ? preference->order : 0;
            
            SearchCatalogInfo info;
            info.addon = addon;
            info.catalog = catalog;
            info.baseUrl = baseUrl;
            info.order = order;
            info.catalogId = catalogId;
            searchCatalogs.append(info);
        }
    }
    
    // Sort by order
    std::sort(searchCatalogs.begin(), searchCatalogs.end());
    
    // Now execute searches in the sorted order
    for (const SearchCatalogInfo& info : searchCatalogs) {
        // Check if this catalog is enabled (default to enabled)
        auto preference = m_catalogPreferencesDao->getPreference(info.addon.id, info.catalog.type(), info.catalogId);
        bool enabled = preference ? preference->enabled : true;
        
        if (!enabled) {
            continue;
        }
        
        m_pendingSearchRequests++;
        
        // Create a new AddonClient for each search request
        AddonClient* client = new AddonClient(info.baseUrl, this);
        m_activeClients.append(client);
        
        // Store search info on client
        client->setProperty("isSearchRequest", true);
        client->setProperty("addonId", info.addon.id);
        client->setProperty("baseUrl", info.baseUrl);
        client->setProperty("searchType", info.catalog.type());
        client->setProperty("searchCatalogId", info.catalog.id());
        client->setProperty("searchQuery", query);
        
        // Connect signals
        connect(client, &AddonClient::searchResultsFetched, this, &LibraryService::onSearchResultsFetched);
        connect(client, &AddonClient::error, this, &LibraryService::onSearchClientError);
        
        // Perform search
        client->search(info.catalog.type(), info.catalog.id(), query);
    }
    
    // If no requests were made, emit empty results
    if (m_pendingSearchRequests == 0) {
        LoggingService::logWarning("LibraryService", "No addons available for search");
        emit searchResultsLoaded(QVariantList());
    }
}

void LibraryService::searchTmdb(const QString& query)
{
    // This method is required by the interface but not currently implemented
    // TMDB search would need to be implemented using TMDB API or addons
    // For now, emit empty results
    LoggingService::logWarning("LibraryService", QString("searchTmdb not implemented for query: %1").arg(query));
    emit searchResultsLoaded(QVariantList());
}


// finishSearch() removed - we now use incremental loading with searchSectionLoaded signal

QVariantList LibraryService::getCatalogSections()
{
    QVariantList sections;
    for (const CatalogSection& section : m_catalogSections) {
        QVariantMap sectionMap;
        sectionMap["name"] = section.name;
        sectionMap["type"] = section.type;
        sectionMap["addonId"] = section.addonId;
        sectionMap["items"] = section.items;
        sections.append(sectionMap);
    }
    return sections;
}

QVariantList LibraryService::getContinueWatching()
{
    return m_continueWatching;
}

void LibraryService::onCatalogFetched(const QString& type, const QJsonArray& metas)
{
    m_pendingCatalogRequests--;
    
    if (metas.isEmpty()) {
        LoggingService::logDebug("LibraryService", QString("Empty catalog received for type: %1").arg(type));
        if (m_pendingCatalogRequests == 0) {
            finishLoadingCatalogs();
        }
        return;
    }
    
    // Find which client sent this
    AddonClient* senderClient = qobject_cast<AddonClient*>(sender());
    if (!senderClient) {
        LoggingService::logWarning("LibraryService", "Could not find sender client");
        if (m_pendingCatalogRequests == 0) {
            finishLoadingCatalogs();
        }
        return;
    }
    
    // Get addon ID and catalog info from client properties
    QString addonId = senderClient->property("addonId").toString();
    QString catalogType = senderClient->property("catalogType").toString();
    QString catalogId = senderClient->property("catalogId").toString();
    QString catalogName = senderClient->property("catalogName").toString();
    
    LoggingService::logDebug("LibraryService", QString("Addon ID from client: %1").arg(addonId));
    LoggingService::logDebug("LibraryService", QString("Catalog info - Type: %1 ID: %2 Name: %3").arg(catalogType, catalogId, catalogName));
    
    if (addonId.isEmpty()) {
        LoggingService::logWarning("LibraryService", "ERROR: Could not find addon ID for catalog");
        if (m_pendingCatalogRequests == 0) {
            finishLoadingCatalogs();
        }
        return;
    }
    
    // Use catalog name from property, or fallback to type
    if (catalogName.isEmpty()) {
        catalogName = catalogType;
    }
    
    // Use catalog type from property, or fallback to signal parameter
    if (catalogType.isEmpty()) {
        catalogType = type;
    }
    
    // If raw export mode, store raw data without processing
    if (m_isRawExport) {
        LoggingService::logDebug("LibraryService", "Raw export mode - storing unprocessed data");
        QVariantMap rawSection;
        rawSection["addonId"] = addonId;
        rawSection["catalogType"] = catalogType;
        rawSection["catalogId"] = catalogId;
        rawSection["catalogName"] = catalogName;
        
        // Convert QJsonArray to QVariantList (raw, unprocessed)
        QVariantList rawItems;
        for (const QJsonValue& value : metas) {
            if (value.isObject()) {
                rawItems.append(value.toObject().toVariantMap());
            }
        }
        rawSection["items"] = rawItems;
        rawSection["itemsCount"] = rawItems.size();
        
        m_rawCatalogData.append(rawSection);
        
        LoggingService::logDebug("LibraryService", QString("Stored raw catalog data - items: %1").arg(rawItems.size()));
    } else {
        LoggingService::logDebug("LibraryService", QString("Processing catalog: %1 type: %2 from addon: %3 items: %4").arg(catalogName, catalogType, addonId).arg(metas.size()));
        processCatalogData(addonId, catalogName, catalogType, metas);
        LoggingService::logDebug("LibraryService", QString("Processed catalog data. Total sections now: %1").arg(m_catalogSections.size()));
    }
    
    if (m_pendingCatalogRequests == 0) {
        LoggingService::logDebug("LibraryService", "All catalog requests completed! Finishing loading...");
        finishLoadingCatalogs();
    } else {
        LoggingService::logDebug("LibraryService", QString("Still waiting for %1 more catalog responses").arg(m_pendingCatalogRequests));
    }
}

void LibraryService::onClientError(const QString& errorMessage)
{
    LoggingService::logDebug("LibraryService", "===== onClientError() called =====");
    LoggingService::logWarning("LibraryService", QString("Client error: %1").arg(errorMessage));
    
    m_pendingCatalogRequests--;
    LoggingService::logDebug("LibraryService", QString("Remaining pending requests: %1").arg(m_pendingCatalogRequests));
    
    if (m_pendingCatalogRequests == 0) {
        LoggingService::logDebug("LibraryService", "All requests completed (with errors). Finishing loading...");
        finishLoadingCatalogs();
    }
}

void LibraryService::onSearchResultsFetched(const QString& type, const QJsonArray& metas)
{
    AddonClient* senderClient = qobject_cast<AddonClient*>(sender());
    if (!senderClient) {
        LoggingService::logWarning("LibraryService", "Could not find sender client for search results");
        m_pendingSearchRequests--;
        return;
    }
    
    QString addonId = senderClient->property("addonId").toString();
    
    m_pendingSearchRequests--;
    
    LoggingService::logDebug("LibraryService", QString("Search results fetched from addon %1, type %2: %3 items").arg(addonId, type).arg(metas.size()));
    
    if (metas.isEmpty()) {
        LoggingService::logDebug("LibraryService", QString("Empty search results for addon: %1, type: %2").arg(addonId, type));
        return;
    }
    
    // Use the actual catalog name from the addon, or fall back to a capitalized type
    // Don't hardcode section names - use what the addon provides
    QString sectionName = type;
    // Capitalize first letter
    if (!sectionName.isEmpty()) {
        sectionName[0] = sectionName[0].toUpper();
    }
    
    // Get base URL for image mapping
    QString baseUrl = senderClient->property("baseUrl").toString();
    
    // Create a section map for the search results
    QVariantMap section;
    section["name"] = sectionName;  // Use actual type-based name, not hardcoded
    section["type"] = type;  // Keep the actual catalog type (movie, series, anime, etc.)
    section["addonId"] = addonId;
    
    // Convert QJsonArray to QVariantList using FrontendDataMapper
    QVariantList items;
    for (const QJsonValue& value : metas) {
        if (value.isObject()) {
            QVariantMap item = FrontendDataMapper::mapCatalogItemToVariantMap(value.toObject(), baseUrl);
            items.append(item);
        }
    }
    section["items"] = items;
    
    // Emit searchSectionLoaded signal for incremental loading
    emit searchSectionLoaded(section);
    
    LoggingService::logDebug("LibraryService", QString("Emitted searchSectionLoaded for %1: %2 items").arg(sectionName).arg(items.size()));
    
    // Check if all search requests are complete
    if (m_pendingSearchRequests == 0) {
        LoggingService::logDebug("LibraryService", "All search requests completed");
        // Search is complete - SearchScreen will handle isLoading state via signal connections
    }
}

void LibraryService::onSearchClientError(const QString& errorMessage)
{
    LoggingService::logWarning("LibraryService", QString("Search client error: %1").arg(errorMessage));
    
    m_pendingSearchRequests--;
    
    // Check if all search requests are complete
    if (m_pendingSearchRequests == 0) {
        LoggingService::logDebug("LibraryService", "All search requests completed");
        // Search is complete - no need to emit anything, SearchScreen will handle isLoading state
    }
}

void LibraryService::onPlaybackProgressFetched(const QVariantList& progress)
{
    LoggingService::logDebug("LibraryService", QString("onPlaybackProgressFetched: received %1 items").arg(progress.size()));
    
    m_continueWatching.clear();
    
    if (progress.isEmpty()) {
        emit continueWatchingLoaded(QVariantList());
        return;
    }
    
    const int watchedThreshold = 81; // More than 80% is considered watched
    
    // Step 1: Filter out items that are >80% watched (>=81%)
    QVariantList filteredItems;
    for (const QVariant& itemVar : progress) {
        QVariantMap item = itemVar.toMap();
        double progressPercent = item["progress"].toDouble();
        if (progressPercent < watchedThreshold) {
            filteredItems.append(item);
        }
    }
    
    LoggingService::logDebug("LibraryService", QString("After filtering >80% watched: %1 items").arg(filteredItems.size()));
    
    // Step 2: For episodes, group by show and keep only the highest episode
    QMap<QString, QVariantMap> showEpisodes; // Key: show title + imdbId, Value: highest episode item
    QVariantList movies;
    
    for (const QVariant& itemVar : filteredItems) {
        QVariantMap item = itemVar.toMap();
        QString type = item["type"].toString();
        
        if (type == "episode") {
            QVariantMap show = item["show"].toMap();
            QVariantMap showIds = show["ids"].toMap();
            QString showImdbId = showIds["imdb"].toString();
            QString showTitle = show["title"].toString();
            QString key = showTitle + "|" + showImdbId;
            
            QVariantMap episode = item["episode"].toMap();
            int season = episode["season"].toInt();
            int episodeNum = episode["number"].toInt();
            
            if (!showEpisodes.contains(key)) {
                // First episode for this show
                showEpisodes[key] = item;
            } else {
                // Compare with existing episode
                QVariantMap existing = showEpisodes[key];
                QVariantMap existingEpisode = existing["episode"].toMap();
                int existingSeason = existingEpisode["season"].toInt();
                int existingEpisodeNum = existingEpisode["number"].toInt();
                
                // Keep the highest episode (higher season, or same season with higher episode)
                if (season > existingSeason || (season == existingSeason && episodeNum > existingEpisodeNum)) {
                    showEpisodes[key] = item;
                }
            }
        } else if (type == "movie") {
            movies.append(item);
        }
    }
    
    LoggingService::logDebug("LibraryService", QString("After grouping episodes: %1 shows, %2 movies").arg(showEpisodes.size()).arg(movies.size()));
    
    // Step 3: Request metadata for continue watching items to enrich with images
    m_continueWatching.clear();
    m_pendingContinueWatchingItems.clear();
    m_pendingContinueWatchingMetadataRequests = 0;
    
    // Process movies
    for (const QVariant& itemVar : movies) {
        QVariantMap item = itemVar.toMap();
        QVariantMap continueItem = traktPlaybackItemToVariantMap(item);
        if (continueItem.isEmpty()) continue;
        
        // Extract contentId for metadata request
        QString contentId = continueItem["imdbId"].toString();
        if (contentId.isEmpty()) {
            // Fallback to tmdbId if imdbId not available
            QString tmdbId = continueItem["tmdbId"].toString();
            if (!tmdbId.isEmpty()) {
                contentId = "tmdb:" + tmdbId;
            }
        }
        
        if (!contentId.isEmpty() && m_mediaMetadataService) {
            // Store item for later merging (store with primary contentId)
            m_pendingContinueWatchingItems[contentId] = continueItem;
            
            // Also store with imdbId if different from contentId (for easier matching)
            QString imdbId = continueItem["imdbId"].toString();
            if (!imdbId.isEmpty() && imdbId != contentId) {
                m_pendingContinueWatchingItems[imdbId] = continueItem;
            }
            
            m_pendingContinueWatchingMetadataRequests++;
            
            // Request metadata
            QString type = continueItem["type"].toString();
            if (type.isEmpty()) type = "movie";
            m_mediaMetadataService->getCompleteMetadata(contentId, type);
        } else {
            // No metadata available, use item as-is
            // Ensure ID field is set - prefer IMDB if available (all addons support it)
            QVariantMap itemToAppend = continueItem;
            if (itemToAppend["id"].toString().isEmpty()) {
                QString imdbId = itemToAppend["imdbId"].toString();
                QString tmdbId = itemToAppend["tmdbId"].toString();
                if (!imdbId.isEmpty()) {
                    itemToAppend["id"] = imdbId;
                } else if (!tmdbId.isEmpty()) {
                    itemToAppend["id"] = "tmdb:" + tmdbId;
                }
            }
            m_continueWatching.append(itemToAppend);
        }
    }
    
    // Process episodes
    for (auto it = showEpisodes.constBegin(); it != showEpisodes.constEnd(); ++it) {
        QVariantMap item = it.value();
        QVariantMap continueItem = traktPlaybackItemToVariantMap(item);
        if (continueItem.isEmpty()) continue;
        
        // Extract contentId for metadata request
        QString contentId = continueItem["imdbId"].toString();
        if (contentId.isEmpty()) {
            // Fallback to tmdbId if imdbId not available
            QString tmdbId = continueItem["tmdbId"].toString();
            if (!tmdbId.isEmpty()) {
                contentId = "tmdb:" + tmdbId;
            }
        }
        
        if (!contentId.isEmpty() && m_mediaMetadataService) {
            // Store item for later merging (store with primary contentId)
            m_pendingContinueWatchingItems[contentId] = continueItem;
            
            // Also store with imdbId if different from contentId (for easier matching)
            QString imdbId = continueItem["imdbId"].toString();
            if (!imdbId.isEmpty() && imdbId != contentId) {
                m_pendingContinueWatchingItems[imdbId] = continueItem;
            }
            
            m_pendingContinueWatchingMetadataRequests++;
            
            // Request metadata (use "series" or "tv" for shows)
            QString type = continueItem["type"].toString();
            if (type.isEmpty() || type == "episode") type = "series";
            m_mediaMetadataService->getCompleteMetadata(contentId, type);
        } else {
            // No metadata available, use item as-is
            // Ensure ID field is set - prefer IMDB if available (all addons support it)
            QVariantMap itemToAppend = continueItem;
            if (itemToAppend["id"].toString().isEmpty()) {
                QString imdbId = itemToAppend["imdbId"].toString();
                QString tmdbId = itemToAppend["tmdbId"].toString();
                if (!imdbId.isEmpty()) {
                    itemToAppend["id"] = imdbId;
                } else if (!tmdbId.isEmpty()) {
                    itemToAppend["id"] = "tmdb:" + tmdbId;
                }
            }
            m_continueWatching.append(itemToAppend);
        }
    }
    
    // If no metadata requests were made, finish immediately
    if (m_pendingContinueWatchingMetadataRequests == 0) {
        finishContinueWatchingLoading();
    }
}

void LibraryService::processCatalogData(const QString& addonId, const QString& catalogName, 
                                       const QString& type, const QJsonArray& metas)
{
    LoggingService::logDebug("LibraryService", "===== processCatalogData() called =====");
    LoggingService::logDebug("LibraryService", QString("Addon ID: %1 Catalog: %2 Type: %3 Metas count: %4").arg(addonId, catalogName, type).arg(metas.size()));
    
    CatalogSection section;
    section.name = catalogName;
    section.type = type;
    section.addonId = addonId;
    
    // Get addon base URL for resolving relative poster URLs
    AddonConfig addon = m_addonRepository->getAddon(addonId);
    QString baseUrl = AddonClient::extractBaseUrl(addon.manifestUrl);
    LoggingService::logDebug("LibraryService", QString("Base URL for resolving images: %1").arg(baseUrl));
    
    int processedItems = 0;
    int skippedItems = 0;
    
    for (const QJsonValue& value : metas) {
        if (value.isObject()) {
            QVariantMap item = FrontendDataMapper::mapCatalogItemToVariantMap(value.toObject(), baseUrl);
            if (!item.isEmpty()) {
                section.items.append(item);
                processedItems++;
                
                // Debug first few items
                if (processedItems <= 3) {
                    LoggingService::logDebug("LibraryService", QString("Processed item %1 - Title: %2 Poster: %3")
                        .arg(processedItems).arg(item["title"].toString()).arg(item["posterUrl"].toString()));
                }
            } else {
                skippedItems++;
            }
        }
    }
    
    LoggingService::logDebug("LibraryService", QString("Processed %1 items, skipped %2 items").arg(processedItems).arg(skippedItems));
    
    if (!section.items.isEmpty()) {
        m_catalogSections.append(section);
        LoggingService::logDebug("LibraryService", QString("✓ Added catalog section: %1 with %2 items").arg(catalogName).arg(section.items.size()));
    } else {
        LoggingService::logDebug("LibraryService", QString("✗ No items in catalog section: %1 - section not added").arg(catalogName));
    }
}

// catalogItemToVariantMap moved to FrontendDataMapper::mapCatalogItemToVariantMap

QVariantMap LibraryService::traktPlaybackItemToVariantMap(const QVariantMap& traktItem)
{
    QVariantMap map;
    
    // Extract type
    QString type = traktItem["type"].toString();
    map["type"] = type;
    
    // Extract progress
    double progress = traktItem["progress"].toDouble();
    map["progress"] = progress;
    map["progressPercent"] = progress; // For progress bar (0-100)
    
    // Extract content data
    QVariantMap movie = traktItem["movie"].toMap();
    QVariantMap show = traktItem["show"].toMap();
    QVariantMap episode = traktItem["episode"].toMap();
    
    if (type == "movie" && !movie.isEmpty()) {
        QVariantMap ids = movie["ids"].toMap();
        map["id"] = ids["imdb"].toString();
        map["imdbId"] = ids["imdb"].toString();
        QString tmdbId = ids["tmdb"].toString();
        if (tmdbId.isEmpty() && ids.contains("tmdb")) {
            int tmdbIdInt = ids["tmdb"].toInt();
            if (tmdbIdInt > 0) {
                tmdbId = QString::number(tmdbIdInt);
            }
        }
        map["tmdbId"] = tmdbId;
        map["title"] = movie["title"].toString();
        map["year"] = movie["year"].toInt();
        
        QVariantMap images = movie["images"].toMap();
        QVariantMap poster = images["poster"].toMap();
        QString posterUrl = poster["full"].toString();
        map["posterUrl"] = posterUrl;
        
        QVariantMap backdrop = images["backdrop"].toMap();
        QString backdropUrl = backdrop["full"].toString();
        // Fallback to poster if backdrop not available
        if (backdropUrl.isEmpty()) {
            backdropUrl = posterUrl;
        }
        map["backdropUrl"] = backdropUrl;
        
        QVariantMap logo = images["logo"].toMap();
        map["logoUrl"] = logo["full"].toString();
        
        LoggingService::logDebug("LibraryService", QString("Movie %1 backdrop: %2 logo: %3")
            .arg(map["title"].toString(), backdropUrl, map["logoUrl"].toString()));
    } else if (type == "episode" && !show.isEmpty() && !episode.isEmpty()) {
        QVariantMap showIds = show["ids"].toMap();
        map["imdbId"] = showIds["imdb"].toString();
        QString tmdbId = showIds["tmdb"].toString();
        if (tmdbId.isEmpty() && showIds.contains("tmdb")) {
            int tmdbIdInt = showIds["tmdb"].toInt();
            if (tmdbIdInt > 0) {
                tmdbId = QString::number(tmdbIdInt);
            }
        }
        map["tmdbId"] = tmdbId;
        map["title"] = show["title"].toString();
        map["year"] = show["year"].toInt();
        map["season"] = episode["season"].toInt();
        map["episode"] = episode["number"].toInt();
        map["episodeTitle"] = episode["title"].toString();
        
        QVariantMap showImages = show["images"].toMap();
        QVariantMap poster = showImages["poster"].toMap();
        QString posterUrl = poster["full"].toString();
        map["posterUrl"] = posterUrl;
        
        // Try episode backdrop first, fallback to show backdrop, then poster
        QVariantMap episodeImages = episode["images"].toMap();
        QVariantMap episodeBackdrop = episodeImages["screenshot"].toMap();
        QString backdropUrl = episodeBackdrop["full"].toString();
        
        if (backdropUrl.isEmpty()) {
            QVariantMap backdrop = showImages["backdrop"].toMap();
            backdropUrl = backdrop["full"].toString();
        }
        
        // Final fallback to poster if no backdrop available
        if (backdropUrl.isEmpty()) {
            backdropUrl = posterUrl;
        }
        
        map["backdropUrl"] = backdropUrl;
        
        QVariantMap logo = showImages["logo"].toMap();
        map["logoUrl"] = logo["full"].toString();
        
        LoggingService::logDebug("LibraryService", QString("Episode %1 S%2 E%3 backdrop: %4 logo: %5")
            .arg(map["title"].toString()).arg(map["season"].toInt()).arg(map["episode"].toInt())
            .arg(backdropUrl, map["logoUrl"].toString()));
    }
    
    // Extract watched_at
    map["watchedAt"] = traktItem["paused_at"].toString();

    // === DATA NORMALIZATION FOR QML COMPATIBILITY ===
    // Ensure ALL expected fields exist with proper types and defaults

    if (map["title"].toString().isEmpty()) {
        map["title"] = "Unknown";
    }

    if (!map.contains("posterUrl")) {
        map["posterUrl"] = "";
    }

    if (!map.contains("backdropUrl")) {
        map["backdropUrl"] = "";
    }

    if (!map.contains("logoUrl")) {
        map["logoUrl"] = "";
    }

    if (!map.contains("type")) {
        map["type"] = "";
    }

    if (!map.contains("season")) {
        map["season"] = 0;
    }

    if (!map.contains("episode")) {
        map["episode"] = 0;
    }

    if (!map.contains("episodeTitle")) {
        map["episodeTitle"] = "";
    }

    if (!map.contains("year") || map["year"].toInt() <= 0) {
        map["year"] = 0;
    }

    if (!map.contains("rating")) {
        map["rating"] = "";
    }

    if (!map.contains("progress")) {
        map["progress"] = 0.0;
    }

    if (!map.contains("progressPercent")) {
        map["progressPercent"] = 0.0;
    }

    if (!map.contains("badgeText")) {
        map["badgeText"] = "";
    }

    if (!map.contains("isHighlighted")) {
        map["isHighlighted"] = false;
    }

    if (!map.contains("imdbId")) {
        map["imdbId"] = "";
    }

    if (!map.contains("type")) {
        map["type"] = "";
    }

    if (!map.contains("season")) {
        map["season"] = 0;
    }

    if (!map.contains("episode")) {
        map["episode"] = 0;
    }

    return map;
}

// continueWatchingItemToVariantMap moved to FrontendDataMapper::mapContinueWatchingItem

void LibraryService::finishLoadingCatalogs()
{
    LoggingService::logDebug("LibraryService", "===== finishLoadingCatalogs() called =====");
    
    m_isLoadingCatalogs = false;
    
    if (m_isRawExport) {
        // Emit raw data
        emit rawCatalogsLoaded(m_rawCatalogData);
        m_isRawExport = false;
        m_rawCatalogData.clear();
    } else {
        // Convert sections to QVariantList
        QVariantList sections;
        int totalItems = 0;
        
        for (const CatalogSection& section : m_catalogSections) {
            QVariantMap sectionMap;
            sectionMap["name"] = section.name;
            sectionMap["type"] = section.type;
            sectionMap["addonId"] = section.addonId;
            sectionMap["items"] = section.items;
            sections.append(sectionMap);
            totalItems += section.items.size();
            
            // Removed debug log - unnecessary
        }
        
        LoggingService::logDebug("LibraryService", QString("Total items across all sections: %1").arg(totalItems));
        emit catalogsLoaded(sections);
    }
    
    LoggingService::logDebug("LibraryService", "✓ Catalog loading finished!");
}

void LibraryService::onHeroCatalogFetched(const QString& type, const QJsonArray& metas)
{
    AddonClient* senderClient = qobject_cast<AddonClient*>(sender());
    if (!senderClient || !senderClient->property("isHeroRequest").toBool()) {
        return; // Not a hero request
    }
    
    m_pendingHeroRequests--;
    
    QString baseUrl = senderClient->property("baseUrl").toString();
    int itemsPerCatalog = senderClient->property("itemsPerCatalog").toInt();
    
    QString addonId = senderClient->property("addonId").toString();

    LoggingService::logCritical("LibraryService", "========== HERO CATALOG FETCHED ==========");
    LoggingService::logCritical("LibraryService", QString("Hero catalog fetched from %1, type: %2, items: %3 taking %4")
        .arg(addonId, type).arg(metas.size()).arg(itemsPerCatalog));
    LoggingService::logCritical("LibraryService", QString("Current hero items count: %1, pending requests: %2")
        .arg(m_heroItems.size()).arg(m_pendingHeroRequests));
    
    int count = 0;
    for (const QJsonValue& value : metas) {
        if (count >= itemsPerCatalog || m_heroItems.size() >= 10) break;
        if (value.isObject()) {
            QVariantMap item = FrontendDataMapper::mapCatalogItemToVariantMap(value.toObject(), baseUrl);
            if (!item.isEmpty()) {
                m_heroItems.append(item);
                count++;
            }
        }
    }
    
    LoggingService::logDebug("LibraryService", QString("Hero items now: %1 pending requests: %2").arg(m_heroItems.size()).arg(m_pendingHeroRequests));
    
    // Clean up client
    m_activeClients.removeAll(senderClient);
    senderClient->deleteLater();
    
    // Emit when we have enough items OR all requests are complete
    if (m_pendingHeroRequests == 0 || m_heroItems.size() >= 10) {
        QVariantList itemsToEmit = m_heroItems.mid(0, 10); // Limit to 10 items
        m_isLoadingHeroItems = false;
        emit heroItemsLoaded(itemsToEmit);
    }
}


void LibraryService::onHeroClientError(const QString& errorMessage)
{
    AddonClient* senderClient = qobject_cast<AddonClient*>(sender());
    if (!senderClient) return;
    
    m_pendingHeroRequests--;
    LoggingService::logWarning("LibraryService", QString("Error loading hero catalog: %1").arg(errorMessage));
    
    // Clean up client
    m_activeClients.removeAll(senderClient);
    senderClient->deleteLater();
    
    // If all requests completed, emit what we have
    if (m_pendingHeroRequests == 0) {
        m_isLoadingHeroItems = false;
        LoggingService::logDebug("LibraryService", QString("Emitting hero items (after error): %1").arg(m_heroItems.size()));
        emit heroItemsLoaded(m_heroItems.mid(0, 10));
    }
}


void LibraryService::finishContinueWatchingLoading()
{
    LoggingService::logDebug("LibraryService", QString("Finishing continue watching loading, items: %1").arg(m_continueWatching.size()));
    emit continueWatchingLoaded(m_continueWatching);
    m_pendingContinueWatchingItems.clear();
}

void LibraryService::loadItemDetails(const QString& contentId, const QString& type, const QString& addonId)
{
    LoggingService::logDebug("LibraryService", QString("loadItemDetails called - contentId: %1 type: %2 addonId: %3").arg(contentId, type, addonId));
    
    if (contentId.isEmpty() || type.isEmpty()) {
        LoggingService::report("Missing contentId or type", "MISSING_PARAMS", "LibraryService");
        LoggingService::report("Missing contentId or type", "MISSING_PARAMS", "LibraryService");
        emit error("Missing contentId or type");
        return;
    }
    
    // Store request info
    m_pendingDetailsContentId = contentId;
    m_pendingDetailsType = type;
    m_pendingDetailsAddonId = addonId;
    
    // Use MediaMetadataService to orchestrate TMDB + OMDB
    m_mediaMetadataService->getCompleteMetadata(contentId, type);
}

void LibraryService::onMediaMetadataLoaded(const QVariantMap& details)
{
    LoggingService::logDebug("LibraryService", "Complete metadata loaded from MediaMetadataService");
    
    // Check if this is for continue watching enrichment
    QString contentId = details["id"].toString();
    QString imdbId = details["imdbId"].toString();
    QString tmdbId = details["tmdbId"].toString();
    
    // Try to find matching continue watching item by checking multiple ID formats
    QString matchingKey;
    if (!contentId.isEmpty() && m_pendingContinueWatchingItems.contains(contentId)) {
        matchingKey = contentId;
    } else if (!imdbId.isEmpty() && m_pendingContinueWatchingItems.contains(imdbId)) {
        matchingKey = imdbId;
    } else if (!tmdbId.isEmpty()) {
        QString tmdbKey = "tmdb:" + tmdbId;
        if (m_pendingContinueWatchingItems.contains(tmdbKey)) {
            matchingKey = tmdbKey;
        }
    }
    
    if (!matchingKey.isEmpty()) {
        // This is a continue watching item - merge metadata
        QVariantMap traktItem = m_pendingContinueWatchingItems[matchingKey];
        
        // Merge metadata into continue watching item
        // Preserve Trakt-specific fields (progress, episode info) but enrich with metadata images
        QVariantMap enrichedItem = traktItem;
        
        // Ensure ID field is set - prefer IMDB if available (all addons support it)
        if (enrichedItem["id"].toString().isEmpty()) {
            QString imdbId = enrichedItem["imdbId"].toString();
            QString tmdbId = enrichedItem["tmdbId"].toString();
            if (!imdbId.isEmpty()) {
                enrichedItem["id"] = imdbId;
            } else if (!tmdbId.isEmpty()) {
                enrichedItem["id"] = "tmdb:" + tmdbId;
            }
        }
        
        // Enrich with metadata images if available
        if (details.contains("posterUrl") && !details["posterUrl"].toString().isEmpty()) {
            enrichedItem["posterUrl"] = details["posterUrl"];
        }
        if (details.contains("backdropUrl") && !details["backdropUrl"].toString().isEmpty()) {
            enrichedItem["backdropUrl"] = details["backdropUrl"];
        }
        if (details.contains("logoUrl") && !details["logoUrl"].toString().isEmpty()) {
            enrichedItem["logoUrl"] = details["logoUrl"];
        }
        
        // Add other useful metadata fields
        if (details.contains("description") && enrichedItem["description"].toString().isEmpty()) {
            enrichedItem["description"] = details["description"];
        }
        if (details.contains("genres") && !details["genres"].toList().isEmpty()) {
            enrichedItem["genres"] = details["genres"];
        }
        
        m_continueWatching.append(enrichedItem);
        m_pendingContinueWatchingItems.remove(matchingKey);
        // Also remove by other possible keys to avoid duplicates
        if (!imdbId.isEmpty() && m_pendingContinueWatchingItems.contains(imdbId)) {
            m_pendingContinueWatchingItems.remove(imdbId);
        }
        if (!tmdbId.isEmpty()) {
            QString tmdbKey = "tmdb:" + tmdbId;
            if (m_pendingContinueWatchingItems.contains(tmdbKey)) {
                m_pendingContinueWatchingItems.remove(tmdbKey);
            }
        }
        m_pendingContinueWatchingMetadataRequests--;
        
        LoggingService::logDebug("LibraryService", QString("Enriched continue watching item: %1, remaining: %2")
            .arg(matchingKey).arg(m_pendingContinueWatchingMetadataRequests));
        
        // Check if all metadata requests are complete
        if (m_pendingContinueWatchingMetadataRequests == 0) {
            finishContinueWatchingLoading();
        }
    } else {
        // This is for item details loading
        emit itemDetailsLoaded(details);
    }
    
    // Check if there are pending episode requests for this content
    // Try multiple ID formats to match the request
    QString episodeRequestKey;
    
    LoggingService::logDebug("LibraryService", QString("Checking for pending episode requests - contentId: %1, imdbId: %2, tmdbId: %3")
        .arg(contentId).arg(imdbId).arg(tmdbId));
    LoggingService::logDebug("LibraryService", QString("Pending episode request keys: %1")
        .arg(QStringList(m_pendingSeasonEpisodesRequests.keys()).join(", ")));
    
    // First, try to match by the exact contentId from pending requests
    if (!contentId.isEmpty() && m_pendingSeasonEpisodesRequests.contains(contentId)) {
        episodeRequestKey = contentId;
    } else if (!tmdbId.isEmpty()) {
        QString tmdbKey = "tmdb:" + tmdbId;
        if (m_pendingSeasonEpisodesRequests.contains(tmdbKey)) {
            episodeRequestKey = tmdbKey;
        }
    } else if (!imdbId.isEmpty()) {
        // Try IMDB ID directly
        if (m_pendingSeasonEpisodesRequests.contains(imdbId)) {
            episodeRequestKey = imdbId;
        }
    }
    
    // Also try matching by tmdb: format if we have tmdbId
    if (episodeRequestKey.isEmpty() && !tmdbId.isEmpty()) {
        QString tmdbKey = "tmdb:" + tmdbId;
        if (m_pendingSeasonEpisodesRequests.contains(tmdbKey)) {
            episodeRequestKey = tmdbKey;
        }
    }
    
    if (!episodeRequestKey.isEmpty()) {
        // We have a pending episode request for this metadata
        int seasonNumber = m_pendingSeasonEpisodesRequests.take(episodeRequestKey);
        
        LoggingService::logInfo("LibraryService", QString("Found pending episode request: %1 -> season %2").arg(episodeRequestKey).arg(seasonNumber));
        
        // Try to get episodes using multiple contentId formats
        // Episodes might be stored under IMDB ID even if we requested with TMDB ID
        QVariantList episodes;
        
        // First try the original request key
        episodes = m_mediaMetadataService->getSeriesEpisodes(episodeRequestKey, seasonNumber);
        LoggingService::logDebug("LibraryService", QString("Tried episodes with requestKey '%1': found %2").arg(episodeRequestKey).arg(episodes.size()));
        
        // If not found and we have IMDB ID, try that
        if (episodes.isEmpty() && !imdbId.isEmpty() && episodeRequestKey != imdbId) {
            episodes = m_mediaMetadataService->getSeriesEpisodes(imdbId, seasonNumber);
            LoggingService::logDebug("LibraryService", QString("Tried episodes with IMDB ID '%1': found %2").arg(imdbId).arg(episodes.size()));
        }
        
        // If still not found and we have TMDB ID in different format, try that
        if (episodes.isEmpty() && !tmdbId.isEmpty()) {
            QString tmdbKey = "tmdb:" + tmdbId;
            if (episodeRequestKey != tmdbKey) {
                episodes = m_mediaMetadataService->getSeriesEpisodes(tmdbKey, seasonNumber);
                LoggingService::logDebug("LibraryService", QString("Tried episodes with TMDB key '%1': found %2").arg(tmdbKey).arg(episodes.size()));
            }
        }
        
        // Also try with just the tmdbId as string
        if (episodes.isEmpty() && !tmdbId.isEmpty()) {
            episodes = m_mediaMetadataService->getSeriesEpisodes(tmdbId, seasonNumber);
            LoggingService::logDebug("LibraryService", QString("Tried episodes with tmdbId string '%1': found %2").arg(tmdbId).arg(episodes.size()));
        }
        
        LoggingService::logInfo("LibraryService", QString("Emitting %1 episodes for season %2 (requestKey: %3)")
            .arg(episodes.size()).arg(seasonNumber).arg(episodeRequestKey));
        
        emit seasonEpisodesLoaded(seasonNumber, episodes);
    } else {
        LoggingService::logDebug("LibraryService", "No matching pending episode request found");
    }
}

void LibraryService::onMediaMetadataError(const QString& message)
{
    LoggingService::report(message, "LIBRARY_ERROR", "LibraryService");
    
    // If we have pending continue watching requests, decrement counter
    // We can't identify which specific request failed, so we'll just count down
    // This is a best-effort approach - if metadata fails, we'll use the item without enrichment
    if (m_pendingContinueWatchingMetadataRequests > 0) {
        m_pendingContinueWatchingMetadataRequests--;
        LoggingService::logWarning("LibraryService", QString("Metadata request failed for continue watching, remaining: %1")
            .arg(m_pendingContinueWatchingMetadataRequests));
        
        // If all requests are done (or failed), finish loading
        if (m_pendingContinueWatchingMetadataRequests == 0) {
            finishContinueWatchingLoading();
        }
    } else {
        // This was for item details, emit error
        emit error(message);
    }
}

void LibraryService::onWatchProgressLoaded(const QVariantMap& progress)
{
    QString contentId = progress["contentId"].toString(); // This is now TMDB ID for smart play
    QString type = progress["type"].toString();

    LoggingService::logDebug("LibraryService", QString("onWatchProgressLoaded for TMDB ID %1 type: %2").arg(contentId, type));
    LoggingService::logDebug("LibraryService", QString("onWatchProgressLoaded - hasProgress: %1 isWatched: %2 progress: %3")
        .arg(progress["hasProgress"].toString()).arg(progress["isWatched"].toString()).arg(progress["progress"].toString()));
    LoggingService::logDebug("LibraryService", QString("onWatchProgressLoaded - pendingSmartPlayItems keys: %1").arg(QStringList(m_pendingSmartPlayItems.keys()).join(", ")));

    // Check if this was for smart play (contentId is now TMDB ID)
    if (!m_pendingSmartPlayItems.contains(contentId)) {
        LoggingService::logDebug("LibraryService", "onWatchProgressLoaded: Not for smart play, ignoring");
        return; // Not for smart play, ignore
    }

    QVariantMap itemData = m_pendingSmartPlayItems.take(contentId);
    QVariantMap smartPlayState = progress;

    // Default values
    smartPlayState["buttonText"] = "Play";
    smartPlayState["action"] = "play";
    smartPlayState["season"] = -1;
    smartPlayState["episode"] = -1;

    bool hasProgress = progress["hasProgress"].toBool();
    double watchProgress = progress["progress"].toDouble();

    LoggingService::logDebug("LibraryService", QString("Processing smart play state - type: %1 hasProgress: %2 watchProgress: %3 isWatched: %4")
        .arg(type).arg(hasProgress).arg(watchProgress).arg(progress["isWatched"].toString()));

    if (type == "movie") {
        if (!hasProgress) {
            // Not watched at all
            smartPlayState["buttonText"] = "Play";
            smartPlayState["action"] = "play";
        } else if (watchProgress < 0.95) {
            // Partially watched
            smartPlayState["buttonText"] = "Continue";
            smartPlayState["action"] = "continue";
        } else {
            // Fully watched
            smartPlayState["buttonText"] = "Rewatch";
            smartPlayState["action"] = "rewatch";
        }
    } else if (type == "tv") {
        int lastWatchedSeason = progress["lastWatchedSeason"].toInt();
        int lastWatchedEpisode = progress["lastWatchedEpisode"].toInt();
        bool isWatched = progress["isWatched"].toBool();

        // Get episode information from itemData
        QVariantList seasons = itemData["seasons"].toList();
        QDateTime now = QDateTime::currentDateTimeUtc();

        if (!hasProgress) {
            // Not watched at all
            smartPlayState["buttonText"] = "Play";
            smartPlayState["action"] = "play";
            smartPlayState["season"] = 1;
            smartPlayState["episode"] = 1;
        } else if (isWatched && lastWatchedSeason > 0 && lastWatchedEpisode > 0) {
            // Find next episode after the last watched one
            bool foundNext = false;
            for (int s = lastWatchedSeason; s <= seasons.size() && !foundNext; ++s) {
                QVariantMap season = seasons[s-1].toMap();
                QVariantList episodes = season["episodes"].toList();

                int startEpisode = (s == lastWatchedSeason) ? lastWatchedEpisode + 1 : 1;

                for (int e = startEpisode; e <= episodes.size() && !foundNext; ++e) {
                    QVariantMap episode = episodes[e-1].toMap();
                    QString airDateStr = episode["air_date"].toString();

                    if (!airDateStr.isEmpty()) {
                        QDateTime airDate = QDateTime::fromString(airDateStr, Qt::ISODate);
                        if (airDate.isValid()) {
                            if (airDate <= now) {
                                // Next episode is available
                                smartPlayState["buttonText"] = QString("Play S%1:E%2").arg(s).arg(e);
                                smartPlayState["action"] = "play";
                                smartPlayState["season"] = s;
                                smartPlayState["episode"] = e;
                                foundNext = true;
                            } else {
                                // Next episode not yet aired
                                smartPlayState["buttonText"] = "Soon";
                                smartPlayState["action"] = "soon";
                                smartPlayState["season"] = s;
                                smartPlayState["episode"] = e;
                                foundNext = true;
                            }
                        }
                    }
                }
            }

            if (!foundNext) {
                // No more episodes, fully watched
                smartPlayState["buttonText"] = "Rewatch";
                smartPlayState["action"] = "rewatch";
            }
        } else if (!isWatched && lastWatchedSeason > 0 && lastWatchedEpisode > 0) {
            // Partially watched current episode
            smartPlayState["buttonText"] = QString("Continue S%1:E%2").arg(lastWatchedSeason).arg(lastWatchedEpisode);
            smartPlayState["action"] = "continue";
            smartPlayState["season"] = lastWatchedSeason;
            smartPlayState["episode"] = lastWatchedEpisode;
        } else {
            // Fallback to play first episode
            smartPlayState["buttonText"] = "Play";
            smartPlayState["action"] = "play";
            smartPlayState["season"] = 1;
            smartPlayState["episode"] = 1;
        }
    }

    emit smartPlayStateLoaded(smartPlayState);
}


void LibraryService::getSmartPlayState(const QVariantMap& itemData)
{
    LoggingService::logDebug("LibraryService", "===== getSmartPlayState CALLED =====");
    
    // Extract any available ID from itemData (addons can provide any ID type)
    QString idToMatch;
    
    // Try different ID fields in order of preference
    if (!itemData["imdbId"].toString().isEmpty()) {
        idToMatch = itemData["imdbId"].toString();
    } else if (!itemData["tmdbId"].toString().isEmpty()) {
        idToMatch = itemData["tmdbId"].toString();
    } else if (!itemData["tvdbId"].toString().isEmpty()) {
        idToMatch = itemData["tvdbId"].toString();
    } else if (!itemData["traktId"].toString().isEmpty()) {
        idToMatch = itemData["traktId"].toString();
    } else if (!itemData["contentId"].toString().isEmpty()) {
        idToMatch = itemData["contentId"].toString();
    } else if (!itemData["id"].toString().isEmpty()) {
        idToMatch = itemData["id"].toString();
    }

    QString type = itemData["type"].toString();
    if (type.isEmpty()) {
        type = itemData["media_type"].toString();
    }

    // Normalize type - database uses "movie" and "tv"
    if (type == "tv" || type == "series") {
        type = "tv";
    } else if (type == "movie") {
        type = "movie";
    } else {
        // If type is empty or unknown, try to infer from itemData
        LoggingService::logDebug("LibraryService", QString("getSmartPlayState: Unknown type: %1, defaulting to movie").arg(type));
        type = "movie";
    }
    
    LoggingService::logDebug("LibraryService", QString("getSmartPlayState - idToMatch: %1 type: %2").arg(idToMatch, type));
    LoggingService::logDebug("LibraryService", QString("getSmartPlayState - itemData keys: %1").arg(QStringList(itemData.keys()).join(", ")));

    if (idToMatch.isEmpty()) {
        LoggingService::logDebug("LibraryService", "getSmartPlayState: No ID found in itemData (checked imdbId, tmdbId, tvdbId, traktId, contentId, id), cannot check watch history");
        // Emit default state
        QVariantMap defaultState;
        defaultState["buttonText"] = "Play";
        defaultState["action"] = "play";
        defaultState["season"] = -1;
        defaultState["episode"] = -1;
        emit smartPlayStateLoaded(defaultState);
        return;
    }

    // Use contentId as key for pending items (fallback to idToMatch)
    QString contentId = itemData["contentId"].toString();
    if (contentId.isEmpty()) {
        contentId = idToMatch;
    }
    
    // Store the item data for processing when progress is received (use contentId as key)
    m_pendingSmartPlayItems[contentId] = itemData;
    LoggingService::logDebug("LibraryService", QString("Stored pending item for contentId: %1").arg(contentId));

    // Get watch progress from local library service using any ID
    // LocalLibraryService will use getWatchHistoryByAnyId to match against all stored IDs
    LoggingService::logDebug("LibraryService", QString("Calling getWatchProgress with id: %1 type: %2").arg(idToMatch, type));
    m_localLibraryService->getWatchProgress(idToMatch, type);
}


void LibraryService::clearMetadataCache()
{
    if (m_mediaMetadataService) {
        m_mediaMetadataService->clearMetadataCache();
    }
    LoggingService::logDebug("LibraryService", "Metadata cache cleared");
}

int LibraryService::getMetadataCacheSize() const
{
    if (m_mediaMetadataService) {
        return m_mediaMetadataService->getMetadataCacheSize();
    }
    return 0;
}

void LibraryService::loadSimilarItems(int tmdbId, const QString& type)
{
    // This method is required by the interface but not currently implemented
    // Similar items would need to be fetched from TMDB API or addons
    // For now, emit empty results
    LoggingService::logWarning("LibraryService", QString("loadSimilarItems not implemented for tmdbId: %1, type: %2").arg(tmdbId).arg(type));
    emit similarItemsLoaded(QVariantList());
}

void LibraryService::loadSeasonEpisodes(const QString& contentId, int seasonNumber)
{
    LoggingService::logInfo("LibraryService", QString("Loading episodes for contentId: %1, season: %2").arg(contentId).arg(seasonNumber));
    
    if (!m_mediaMetadataService) {
        LoggingService::logWarning("LibraryService", "MediaMetadataService not available");
        emit seasonEpisodesLoaded(seasonNumber, QVariantList());
        return;
    }
    
    if (contentId.isEmpty()) {
        LoggingService::logWarning("LibraryService", "Empty contentId provided for loadSeasonEpisodes");
        emit seasonEpisodesLoaded(seasonNumber, QVariantList());
        return;
    }
    
    // First, try to get episodes from cache (if metadata was already fetched)
    QVariantList episodes = m_mediaMetadataService->getSeriesEpisodes(contentId, seasonNumber);
    
    if (!episodes.isEmpty()) {
        // Episodes found in cache, emit them
        LoggingService::logInfo("LibraryService", QString("Found %1 episodes in cache for season %2").arg(episodes.size()).arg(seasonNumber));
        emit seasonEpisodesLoaded(seasonNumber, episodes);
        return;
    }
    
    // Episodes not in cache, need to fetch metadata first
    // This will extract episodes from the metadata response
    LoggingService::logInfo("LibraryService", QString("Episodes not in cache, fetching metadata for contentId: %1").arg(contentId));
    
    // Store pending request info - we'll check this when metadata loads
    m_pendingSeasonEpisodesRequests[contentId] = seasonNumber;
    LoggingService::logDebug("LibraryService", QString("Stored pending episode request: %1 -> season %2").arg(contentId).arg(seasonNumber));
    
    // Fetch metadata using the provided contentId
    // When metadata loads, onMediaMetadataLoaded will check for pending episode requests
    m_mediaMetadataService->getCompleteMetadata(contentId, "tv");
}

void LibraryService::loadSeasonEpisodes(int tmdbId, int seasonNumber)
{
    // Legacy overload for QML compatibility - converts TMDB ID to contentId format
    QString contentId = QString("tmdb:%1").arg(tmdbId);
    loadSeasonEpisodes(contentId, seasonNumber);
}

