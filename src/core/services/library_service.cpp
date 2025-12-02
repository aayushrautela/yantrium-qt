#include "library_service.h"
#include "core/database/database_manager.h"
#include "core/services/id_parser.h"
#include "core/services/configuration.h"
#include "core/services/frontend_data_mapper.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

LibraryService::LibraryService(QObject* parent)
    : QObject(parent)
    , m_addonRepository(new AddonRepository(this))
    , m_traktService(&TraktCoreService::instance())
    , m_tmdbService(new TmdbDataService(this))
    , m_mediaMetadataService(new MediaMetadataService(this))
    , m_omdbService(new OmdbService(this))
    , m_catalogPreferencesDao(new CatalogPreferencesDao(DatabaseManager::instance().database()))
    , m_pendingCatalogRequests(0)
    , m_isLoadingCatalogs(false)
    , m_isRawExport(false)
    , m_pendingHeroRequests(0)
    , m_isLoadingHeroItems(false)
    , m_pendingTmdbRequests(0)
{
    qDebug() << "[LibraryService] ===== Constructor called =====";
    qDebug() << "[LibraryService] LibraryService instance created";
    
    // Connect to Trakt service for continue watching
    connect(m_traktService, &TraktCoreService::playbackProgressFetched,
            this, &LibraryService::onPlaybackProgressFetched);
    
    // Connect to TMDB service for continue watching images
    connect(m_tmdbService, &TmdbDataService::movieMetadataFetched,
            this, &LibraryService::onTmdbMovieMetadataFetched);
    connect(m_tmdbService, &TmdbDataService::tvMetadataFetched,
            this, &LibraryService::onTmdbTvMetadataFetched);
    connect(m_tmdbService, &TmdbDataService::tmdbIdFound,
            this, &LibraryService::onTmdbIdFound);
    connect(m_tmdbService, &TmdbDataService::error,
            this, &LibraryService::onTmdbError);
    connect(m_tmdbService, &TmdbDataService::similarMoviesFetched,
            this, &LibraryService::onSimilarMoviesFetched);
    connect(m_tmdbService, &TmdbDataService::similarTvFetched,
            this, &LibraryService::onSimilarTvFetched);
    
    // Connect to MediaMetadataService for item details
    connect(m_mediaMetadataService, &MediaMetadataService::metadataLoaded,
            this, &LibraryService::onMediaMetadataLoaded);
    connect(m_mediaMetadataService, &MediaMetadataService::error,
            this, &LibraryService::onMediaMetadataError);
    
    qDebug() << "[LibraryService] Connected to Trakt, TMDB, MediaMetadata, and OMDB service signals";
}

LibraryService::~LibraryService()
{
    // Clean up active clients
    qDeleteAll(m_activeClients);
    m_activeClients.clear();
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
    qDebug() << "[LibraryService] ===== loadCatalogs() called (rawMode:" << rawMode << ") =====";
    
    if (m_isLoadingCatalogs) {
        qDebug() << "[LibraryService] Already loading catalogs, skipping";
        return;
    }
    
    m_isLoadingCatalogs = true;
    m_isRawExport = rawMode;
    m_catalogSections.clear();
    m_rawCatalogData.clear();
    m_pendingCatalogRequests = 0;
    
    qDebug() << "[LibraryService] Cleared catalog sections and reset pending requests (rawMode:" << rawMode << ")";
    
    // Clean up old clients
    qDeleteAll(m_activeClients);
    m_activeClients.clear();
    qDebug() << "[LibraryService] Cleared old clients";
    
    // Get enabled addons
    QList<AddonConfig> enabledAddons = m_addonRepository->getEnabledAddons();
    qDebug() << "[LibraryService] Found" << enabledAddons.size() << "enabled addons";
    
    if (enabledAddons.isEmpty()) {
        qDebug() << "[LibraryService] ERROR: No enabled addons found - cannot load catalogs";
        m_isLoadingCatalogs = false;
        emit catalogsLoaded(QVariantList());
        return;
    }
    
    qDebug() << "[LibraryService] Loading catalogs from" << enabledAddons.size() << "enabled addons";
    
    // For each enabled addon, fetch its catalogs
    for (const AddonConfig& addon : enabledAddons) {
        qDebug() << "[LibraryService] Processing addon:" << addon.id() << "(" << addon.name() << ")";
        
        AddonManifest manifest = m_addonRepository->getManifest(addon);
        if (manifest.id().isEmpty()) {
            qDebug() << "[LibraryService] ERROR: Could not get manifest for addon:" << addon.id();
            continue;
        }
        
        qDebug() << "[LibraryService] Got manifest for addon:" << addon.id();
        
        // Get catalogs from manifest
        QList<CatalogDefinition> catalogs = manifest.catalogs();
        qDebug() << "[LibraryService] Found" << catalogs.size() << "catalog definitions in manifest for addon:" << addon.id();
        
        if (catalogs.isEmpty()) {
            qDebug() << "[LibraryService] WARNING: No catalogs found for addon:" << addon.id();
            continue;
        }
        
        // Get base URL for this addon
        QString baseUrl = AddonClient::extractBaseUrl(addon.manifestUrl());
        qDebug() << "[LibraryService] Extracted base URL:" << baseUrl << "for addon:" << addon.id();
        
        int enabledCatalogCount = 0;
        int disabledCatalogCount = 0;
        
        // Fetch each catalog (only if enabled)
        // IMPORTANT: Create a separate AddonClient for each catalog request
        // because AddonClient only supports one request at a time (single m_currentReply)
        for (const CatalogDefinition& catalog : catalogs) {
            QString catalogName = catalog.name().isEmpty() ? catalog.type() : catalog.name();
            QString catalogId = catalog.id();
            QString catalogType = catalog.type();
            
            qDebug() << "[LibraryService] Checking catalog:" << catalogName << "type:" << catalogType << "id:" << catalogId;
            
            // Check if this catalog is enabled (default to enabled if no preference exists)
            auto preference = m_catalogPreferencesDao->getPreference(addon.id(), catalogType, catalogId);
            bool isEnabled = preference ? preference->enabled : true;
            
            qDebug() << "[LibraryService] Catalog preference check - enabled:" << isEnabled 
                     << "(preference exists:" << (preference != nullptr) << ")";
            
            if (!isEnabled) {
                disabledCatalogCount++;
                qDebug() << "[LibraryService] SKIPPING disabled catalog:" << catalogName << "(" << catalogType << "," << catalogId << ") from addon:" << addon.id();
                continue;
            }
            
            enabledCatalogCount++;
            m_pendingCatalogRequests++;
            qDebug() << "[LibraryService] FETCHING catalog:" << catalogName << "(" << catalogType << "," << catalogId << ") from addon:" << addon.id();
            qDebug() << "[LibraryService] Pending requests now:" << m_pendingCatalogRequests;
            
            // Create a NEW AddonClient for each catalog request
            // This is necessary because AddonClient only supports one concurrent request
            AddonClient* client = new AddonClient(baseUrl, this);
            m_activeClients.append(client);
            
            // Store addon ID and catalog info on client for this specific request
            client->setProperty("addonId", addon.id());
            client->setProperty("baseUrl", baseUrl);
            client->setProperty("catalogType", catalogType);
            client->setProperty("catalogId", catalogId);
            client->setProperty("catalogName", catalogName);
            
            // Connect signals
            connect(client, &AddonClient::catalogFetched, this, &LibraryService::onCatalogFetched);
            connect(client, &AddonClient::error, this, &LibraryService::onClientError);
            
            qDebug() << "[LibraryService] Created new client and calling getCatalog(" << catalogType << "," << catalogId << ")";
            client->getCatalog(catalogType, catalogId);
        }
        
        qDebug() << "[LibraryService] Addon" << addon.id() << "- Enabled catalogs:" << enabledCatalogCount << "Disabled catalogs:" << disabledCatalogCount;
    }
    
    qDebug() << "[LibraryService] Total pending catalog requests:" << m_pendingCatalogRequests;
    
    // Also load continue watching
    qDebug() << "[LibraryService] Requesting continue watching from Trakt";
    m_traktService->getPlaybackProgressWithImages();
    
    // If no requests were made, finish immediately
    if (m_pendingCatalogRequests == 0) {
        qDebug() << "[LibraryService] WARNING: No catalog requests were made! Finishing immediately.";
        finishLoadingCatalogs();
    } else {
        qDebug() << "[LibraryService] Waiting for" << m_pendingCatalogRequests << "catalog responses...";
    }
}

void LibraryService::loadCatalog(const QString& addonId, const QString& type, const QString& id)
{
    AddonConfig addon = m_addonRepository->getAddon(addonId);
    if (addon.id().isEmpty()) {
        emit error(QString("Addon not found: %1").arg(addonId));
        return;
    }
    
    QString baseUrl = AddonClient::extractBaseUrl(addon.manifestUrl());
    AddonClient* client = new AddonClient(baseUrl, this);
    m_activeClients.append(client);
    
    connect(client, &AddonClient::catalogFetched, this, &LibraryService::onCatalogFetched);
    connect(client, &AddonClient::error, this, &LibraryService::onClientError);
    
    client->getCatalog(type, id);
}

void LibraryService::loadHeroItems()
{
    qDebug() << "[LibraryService] ===== loadHeroItems() called =====";
    
    if (m_isLoadingHeroItems) {
        qDebug() << "[LibraryService] Already loading hero items, skipping";
        return;
    }
    
    m_isLoadingHeroItems = true;
    m_heroItems.clear();
    m_pendingHeroRequests = 0;
    
    // Get hero catalogs from preferences
    QList<CatalogPreferenceRecord> heroCatalogs = m_catalogPreferencesDao->getHeroCatalogs();
    
    if (heroCatalogs.isEmpty()) {
        qDebug() << "[LibraryService] No hero catalogs configured, emitting empty list";
        m_isLoadingHeroItems = false;
        emit heroItemsLoaded(QVariantList());
        return;
    }
    
    qDebug() << "[LibraryService] Found" << heroCatalogs.size() << "hero catalogs";
    
    int itemsPerCatalog = qMax(1, 10 / heroCatalogs.size()); // Distribute items across catalogs
    qDebug() << "[LibraryService] Will load" << itemsPerCatalog << "items per catalog";
    
    for (const CatalogPreferenceRecord& heroCatalog : heroCatalogs) {
        try {
            AddonConfig addon = m_addonRepository->getAddon(heroCatalog.addonId);
            if (addon.id().isEmpty()) {
                qWarning() << "[LibraryService] Hero catalog addon not found:" << heroCatalog.addonId;
                continue;
            }
            
            QString baseUrl = AddonClient::extractBaseUrl(addon.manifestUrl());
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
            qDebug() << "[LibraryService] Requesting hero catalog:" << heroCatalog.catalogType << heroCatalog.catalogId;
            client->getCatalog(heroCatalog.catalogType, heroCatalog.catalogId);
            
        } catch (...) {
            qWarning() << "[LibraryService] Exception loading hero catalog:" << heroCatalog.addonId;
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
    
    // For now, we'll search through cached catalog sections
    // In a full implementation, we might want to search addons directly
    QVariantList results;
    QString queryLower = query.toLower();
    
    for (const CatalogSection& section : m_catalogSections) {
        for (const QVariant& itemVar : section.items) {
            QVariantMap item = itemVar.toMap();
            QString title = item["title"].toString().toLower();
            QString description = item["description"].toString().toLower();
            
            if (title.contains(queryLower) || description.contains(queryLower)) {
                results.append(item);
            }
        }
    }
    
    emit searchResultsLoaded(results);
}

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
        qDebug() << "[LibraryService] Empty catalog received for type:" << type;
        if (m_pendingCatalogRequests == 0) {
            finishLoadingCatalogs();
        }
        return;
    }
    
    // Find which client sent this
    AddonClient* senderClient = qobject_cast<AddonClient*>(sender());
    if (!senderClient) {
        qWarning() << "[LibraryService] Could not find sender client";
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
    
    qDebug() << "[LibraryService] Addon ID from client:" << addonId;
    qDebug() << "[LibraryService] Catalog info - Type:" << catalogType << "ID:" << catalogId << "Name:" << catalogName;
    
    if (addonId.isEmpty()) {
        qWarning() << "[LibraryService] ERROR: Could not find addon ID for catalog";
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
        qDebug() << "[LibraryService] Raw export mode - storing unprocessed data";
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
        
        qDebug() << "[LibraryService] Stored raw catalog data - items:" << rawItems.size();
    } else {
        qDebug() << "[LibraryService] Processing catalog:" << catalogName << "type:" << catalogType << "from addon:" << addonId << "items:" << metas.size();
        processCatalogData(addonId, catalogName, catalogType, metas);
        qDebug() << "[LibraryService] Processed catalog data. Total sections now:" << m_catalogSections.size();
    }
    
    if (m_pendingCatalogRequests == 0) {
        qDebug() << "[LibraryService] All catalog requests completed! Finishing loading...";
        finishLoadingCatalogs();
    } else {
        qDebug() << "[LibraryService] Still waiting for" << m_pendingCatalogRequests << "more catalog responses";
    }
}

void LibraryService::onClientError(const QString& errorMessage)
{
    qDebug() << "[LibraryService] ===== onClientError() called =====";
    qWarning() << "[LibraryService] Client error:" << errorMessage;
    
    m_pendingCatalogRequests--;
    qDebug() << "[LibraryService] Remaining pending requests:" << m_pendingCatalogRequests;
    
    if (m_pendingCatalogRequests == 0) {
        qDebug() << "[LibraryService] All requests completed (with errors). Finishing loading...";
        finishLoadingCatalogs();
    }
}

void LibraryService::onPlaybackProgressFetched(const QVariantList& progress)
{
    qDebug() << "[LibraryService] onPlaybackProgressFetched: received" << progress.size() << "items";
    
    m_continueWatching.clear();
    m_pendingContinueWatchingItems.clear();
    m_tmdbIdToImdbId.clear();
    m_pendingTmdbRequests = 0;
    
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
    
    qDebug() << "[LibraryService] After filtering >80% watched:" << filteredItems.size() << "items";
    
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
    
    qDebug() << "[LibraryService] After grouping episodes:" << showEpisodes.size() << "shows," << movies.size() << "movies";
    
    // Step 3: Only fetch TMDB metadata for items that will actually appear
    // Add movies first
    for (const QVariant& itemVar : movies) {
        QVariantMap item = itemVar.toMap();
        QVariantMap movie = item["movie"].toMap();
        QVariantMap ids = movie["ids"].toMap();
        QString imdbId = ids["imdb"].toString();
        
        if (!imdbId.isEmpty()) {
            m_pendingContinueWatchingItems[imdbId] = item;
            m_pendingTmdbRequests++;
            m_tmdbService->getTmdbIdFromImdb(imdbId);
        }
    }
    
    // Add highest episodes
    for (auto it = showEpisodes.constBegin(); it != showEpisodes.constEnd(); ++it) {
        QVariantMap item = it.value();
        QVariantMap show = item["show"].toMap();
        QVariantMap showIds = show["ids"].toMap();
        QString imdbId = showIds["imdb"].toString();
        
        if (!imdbId.isEmpty()) {
            // Debug: Verify episode data is in the item before storing
            if (item.contains("episode")) {
                QVariantMap episode = item["episode"].toMap();
                qDebug() << "[LibraryService] Storing episode item - IMDB:" << imdbId
                         << "S" << episode["season"].toInt() << "E" << episode["number"].toInt()
                         << "Title:" << episode["title"].toString();
            } else {
                qWarning() << "[LibraryService] Episode item missing episode data before storing!";
            }
            
            m_pendingContinueWatchingItems[imdbId] = item;
            m_pendingTmdbRequests++;
            m_tmdbService->getTmdbIdFromImdb(imdbId);
        }
    }
    
    qDebug() << "[LibraryService] Will fetch TMDB metadata for" << m_pendingTmdbRequests << "items";
    
    // If no items to process, emit empty
    if (m_pendingTmdbRequests == 0) {
        emit continueWatchingLoaded(QVariantList());
    }
}

void LibraryService::processCatalogData(const QString& addonId, const QString& catalogName, 
                                       const QString& type, const QJsonArray& metas)
{
    qDebug() << "[LibraryService] ===== processCatalogData() called =====";
    qDebug() << "[LibraryService] Addon ID:" << addonId << "Catalog:" << catalogName << "Type:" << type << "Metas count:" << metas.size();
    
    CatalogSection section;
    section.name = catalogName;
    section.type = type;
    section.addonId = addonId;
    
    // Get addon base URL for resolving relative poster URLs
    AddonConfig addon = m_addonRepository->getAddon(addonId);
    QString baseUrl = AddonClient::extractBaseUrl(addon.manifestUrl());
    qDebug() << "[LibraryService] Base URL for resolving images:" << baseUrl;
    
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
                    qDebug() << "[LibraryService] Processed item" << processedItems << "- Title:" << item["title"].toString() 
                             << "Poster:" << item["posterUrl"].toString();
                }
            } else {
                skippedItems++;
            }
        }
    }
    
    qDebug() << "[LibraryService] Processed" << processedItems << "items, skipped" << skippedItems << "items";
    
    if (!section.items.isEmpty()) {
        m_catalogSections.append(section);
        qDebug() << "[LibraryService] ✓ Added catalog section:" << catalogName << "with" << section.items.size() << "items";
    } else {
        qDebug() << "[LibraryService] ✗ No items in catalog section:" << catalogName << "- section not added";
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
        
        qDebug() << "[LibraryService] Movie" << map["title"] << "backdrop:" << backdropUrl << "logo:" << map["logoUrl"].toString();
    } else if (type == "episode" && !show.isEmpty() && !episode.isEmpty()) {
        QVariantMap showIds = show["ids"].toMap();
        map["imdbId"] = showIds["imdb"].toString();
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
        
        qDebug() << "[LibraryService] Episode" << map["title"] << "S" << map["season"] << "E" << map["episode"] 
                 << "backdrop:" << backdropUrl << "logo:" << map["logoUrl"].toString();
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
    qDebug() << "[LibraryService] ===== finishLoadingCatalogs() called =====";
    
    m_isLoadingCatalogs = false;
    
    if (m_isRawExport) {
        // Emit raw data
        qDebug() << "[LibraryService] Emitting rawCatalogsLoaded signal with" << m_rawCatalogData.size() << "sections";
        emit rawCatalogsLoaded(m_rawCatalogData);
        m_isRawExport = false;
        m_rawCatalogData.clear();
    } else {
        // Convert sections to QVariantList
        QVariantList sections;
        int totalItems = 0;
        
        qDebug() << "[LibraryService] Total catalog sections:" << m_catalogSections.size();
        
        for (const CatalogSection& section : m_catalogSections) {
            QVariantMap sectionMap;
            sectionMap["name"] = section.name;
            sectionMap["type"] = section.type;
            sectionMap["addonId"] = section.addonId;
            sectionMap["items"] = section.items;
            sections.append(sectionMap);
            totalItems += section.items.size();
            
            qDebug() << "[LibraryService] Section:" << section.name << "(" << section.type << ") from addon:" << section.addonId 
                     << "has" << section.items.size() << "items";
        }
        
        qDebug() << "[LibraryService] Total items across all sections:" << totalItems;
        qDebug() << "[LibraryService] Emitting catalogsLoaded signal with" << sections.size() << "sections";
        qDebug() << "[LibraryService] Signal receivers count:" << receivers(SIGNAL(catalogsLoaded(QVariantList)));
        emit catalogsLoaded(sections);
        qDebug() << "[LibraryService] Signal emitted";
    }
    
    qDebug() << "[LibraryService] ✓ Catalog loading finished!";
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
    
    qDebug() << "[LibraryService] Hero catalog fetched, items:" << metas.size() << "taking" << itemsPerCatalog;
    
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
    
    qDebug() << "[LibraryService] Hero items now:" << m_heroItems.size() << "pending requests:" << m_pendingHeroRequests;
    
    // Clean up client
    m_activeClients.removeAll(senderClient);
    senderClient->deleteLater();
    
    // If all requests completed or we have enough items, emit signal
    if (m_pendingHeroRequests == 0 || m_heroItems.size() >= 10) {
        m_isLoadingHeroItems = false;
        QVariantList itemsToEmit = m_heroItems.mid(0, 10); // Limit to 10 items
        qDebug() << "[LibraryService] Emitting hero items loaded:" << itemsToEmit.size();
        emit heroItemsLoaded(itemsToEmit);
    }
}

void LibraryService::onHeroClientError(const QString& errorMessage)
{
    AddonClient* senderClient = qobject_cast<AddonClient*>(sender());
    if (!senderClient) return;
    
    m_pendingHeroRequests--;
    qWarning() << "[LibraryService] Error loading hero catalog:" << errorMessage;
    
    // Clean up client
    m_activeClients.removeAll(senderClient);
    senderClient->deleteLater();
    
    // If all requests completed, emit what we have
    if (m_pendingHeroRequests == 0) {
        m_isLoadingHeroItems = false;
        qDebug() << "[LibraryService] Emitting hero items (after error):" << m_heroItems.size();
        emit heroItemsLoaded(m_heroItems.mid(0, 10));
    }
}

void LibraryService::onTmdbIdFound(const QString& imdbId, int tmdbId)
{
    qDebug() << "[LibraryService] TMDB ID found for IMDB" << imdbId << "-> TMDB" << tmdbId;
    
    if (!m_pendingContinueWatchingItems.contains(imdbId)) {
        qWarning() << "[LibraryService] Received TMDB ID for unknown IMDB ID:" << imdbId;
        m_pendingTmdbRequests--;
        if (m_pendingTmdbRequests == 0) {
            finishContinueWatchingLoading();
        }
        return;
    }
    
    QVariantMap traktItem = m_pendingContinueWatchingItems[imdbId];
    QString type = traktItem["type"].toString();
    
    m_tmdbIdToImdbId[tmdbId] = imdbId;
    
    // Fetch TMDB metadata
    if (type == "movie") {
        m_tmdbService->getMovieMetadata(tmdbId);
    } else if (type == "episode") {
        m_tmdbService->getTvMetadata(tmdbId);
    } else {
        qWarning() << "[LibraryService] Unknown type for TMDB fetch:" << type;
        m_pendingTmdbRequests--;
        if (m_pendingTmdbRequests == 0) {
            finishContinueWatchingLoading();
        }
    }
}

void LibraryService::onTmdbMovieMetadataFetched(int tmdbId, const QJsonObject& data)
{
    qDebug() << "[LibraryService] TMDB movie metadata fetched for TMDB ID:" << tmdbId;
    
    if (!m_tmdbIdToImdbId.contains(tmdbId)) {
        qWarning() << "[LibraryService] Received movie metadata for unknown TMDB ID:" << tmdbId;
        m_pendingTmdbRequests--;
        if (m_pendingTmdbRequests == 0) {
            finishContinueWatchingLoading();
        }
        return;
    }
    
    QString imdbId = m_tmdbIdToImdbId[tmdbId];
    if (!m_pendingContinueWatchingItems.contains(imdbId)) {
        qWarning() << "[LibraryService] Movie metadata for unknown IMDB ID:" << imdbId;
        m_pendingTmdbRequests--;
        if (m_pendingTmdbRequests == 0) {
            finishContinueWatchingLoading();
        }
        return;
    }
    
    QVariantMap traktItem = m_pendingContinueWatchingItems[imdbId];
    QVariantMap continueItem = FrontendDataMapper::mapContinueWatchingItem(traktItem, data);
    
    if (!continueItem.isEmpty()) {
        m_continueWatching.append(continueItem);
    }
    
    m_pendingTmdbRequests--;
    if (m_pendingTmdbRequests == 0) {
        finishContinueWatchingLoading();
    }
}

void LibraryService::onTmdbTvMetadataFetched(int tmdbId, const QJsonObject& data)
{
    qDebug() << "[LibraryService] TMDB TV metadata fetched for TMDB ID:" << tmdbId;
    
    if (!m_tmdbIdToImdbId.contains(tmdbId)) {
        qWarning() << "[LibraryService] Received TV metadata for unknown TMDB ID:" << tmdbId;
        m_pendingTmdbRequests--;
        if (m_pendingTmdbRequests == 0) {
            finishContinueWatchingLoading();
        }
        return;
    }
    
    QString imdbId = m_tmdbIdToImdbId[tmdbId];
    if (!m_pendingContinueWatchingItems.contains(imdbId)) {
        qWarning() << "[LibraryService] TV metadata for unknown IMDB ID:" << imdbId;
        m_pendingTmdbRequests--;
        if (m_pendingTmdbRequests == 0) {
            finishContinueWatchingLoading();
        }
        return;
    }
    
    QVariantMap traktItem = m_pendingContinueWatchingItems[imdbId];
    
    // Debug: Log episode data from Trakt
    if (traktItem.contains("episode")) {
        QVariantMap episode = traktItem["episode"].toMap();
        qDebug() << "[LibraryService] Trakt episode data - season:" << episode["season"].toInt() 
                 << "episode:" << episode["number"].toInt() 
                 << "title:" << episode["title"].toString();
    } else {
        qWarning() << "[LibraryService] Trakt item missing episode data!";
    }
    
    QVariantMap continueItem = FrontendDataMapper::mapContinueWatchingItem(traktItem, data);
    
    // Debug: Log what we extracted
    qDebug() << "[LibraryService] Continue watching item - season:" << continueItem["season"].toInt()
             << "episode:" << continueItem["episode"].toInt()
             << "episodeTitle:" << continueItem["episodeTitle"].toString()
             << "title:" << continueItem["title"].toString();
    
    if (!continueItem.isEmpty()) {
        m_continueWatching.append(continueItem);
    }
    
    m_pendingTmdbRequests--;
    if (m_pendingTmdbRequests == 0) {
        finishContinueWatchingLoading();
    }
}

void LibraryService::onTmdbError(const QString& message)
{
    qWarning() << "[LibraryService] TMDB error:" << message;
    // Decrement pending requests and finish if all done
    m_pendingTmdbRequests--;
    if (m_pendingTmdbRequests == 0) {
        finishContinueWatchingLoading();
    }
}

void LibraryService::finishContinueWatchingLoading()
{
    qDebug() << "[LibraryService] Finishing continue watching loading, items:" << m_continueWatching.size();
    emit continueWatchingLoaded(m_continueWatching);
    m_pendingContinueWatchingItems.clear();
    m_tmdbIdToImdbId.clear();
}

void LibraryService::loadItemDetails(const QString& contentId, const QString& type, const QString& addonId)
{
    qDebug() << "[LibraryService] loadItemDetails called - contentId:" << contentId << "type:" << type << "addonId:" << addonId;
    
    if (contentId.isEmpty() || type.isEmpty()) {
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
    qDebug() << "[LibraryService] Complete metadata loaded from MediaMetadataService";
    emit itemDetailsLoaded(details);
}

void LibraryService::onMediaMetadataError(const QString& message)
{
    qWarning() << "[LibraryService] MediaMetadataService error:" << message;
    emit error(message);
}

void LibraryService::loadSimilarItems(int tmdbId, const QString& type)
{
    qDebug() << "[LibraryService] Loading similar items for TMDB ID:" << tmdbId << "type:" << type;
    
    if (type == "movie") {
        m_tmdbService->getSimilarMovies(tmdbId);
    } else if (type == "series" || type == "tv") {
        m_tmdbService->getSimilarTv(tmdbId);
    } else {
        emit error(QString("Unknown type for similar items: %1").arg(type));
    }
}

void LibraryService::onSimilarMoviesFetched(int tmdbId, const QJsonArray& results)
{
    qDebug() << "[LibraryService] Similar movies fetched for TMDB ID:" << tmdbId << "count:" << results.size();
    
    QVariantList items = FrontendDataMapper::mapSimilarItemsToVariantList(results, "movie");
    emit similarItemsLoaded(items);
}

void LibraryService::onSimilarTvFetched(int tmdbId, const QJsonArray& results)
{
    qDebug() << "[LibraryService] Similar TV shows fetched for TMDB ID:" << tmdbId << "count:" << results.size();
    
    QVariantList items = FrontendDataMapper::mapSimilarItemsToVariantList(results, "series");
    emit similarItemsLoaded(items);
}

