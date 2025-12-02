#include "library_service.h"
#include "core/database/database_manager.h"
#include "core/services/id_parser.h"
#include "core/services/configuration.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

LibraryService::LibraryService(QObject* parent)
    : QObject(parent)
    , m_addonRepository(new AddonRepository(this))
    , m_traktService(&TraktCoreService::instance())
    , m_tmdbService(new TmdbService(this))
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
    
    // Connect to TMDB service for fetching images
    connect(m_tmdbService, &TmdbService::movieMetadataFetched,
            this, &LibraryService::onTmdbMovieMetadataFetched);
    connect(m_tmdbService, &TmdbService::tvMetadataFetched,
            this, &LibraryService::onTmdbTvMetadataFetched);
    connect(m_tmdbService, &TmdbService::tmdbIdFound,
            this, &LibraryService::onTmdbIdFound);
    connect(m_tmdbService, &TmdbService::error,
            this, &LibraryService::onTmdbError);
    connect(m_tmdbService, &TmdbService::similarMoviesFetched,
            this, &LibraryService::onSimilarMoviesFetched);
    connect(m_tmdbService, &TmdbService::similarTvFetched,
            this, &LibraryService::onSimilarTvFetched);
    
    qDebug() << "[LibraryService] Connected to Trakt and TMDB service signals";
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
            QVariantMap item = catalogItemToVariantMap(value.toObject(), baseUrl);
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

QVariantMap LibraryService::catalogItemToVariantMap(const QJsonObject& item, const QString& baseUrl)
{
    QVariantMap map;
    
    // Helper function to resolve relative URLs
    auto resolveUrl = [&baseUrl](const QString& url) -> QString {
        if (url.isEmpty()) {
            return url;
        }
        // If already absolute, return as-is
        if (url.startsWith("http://") || url.startsWith("https://")) {
            return url;
        }
        // If relative, resolve against base URL
        if (!baseUrl.isEmpty() && url.startsWith("/")) {
            // Absolute path relative to base URL
            QString normalizedBase = baseUrl;
            if (normalizedBase.endsWith('/')) {
                normalizedBase.chop(1);
            }
            return normalizedBase + url;
        } else if (!baseUrl.isEmpty() && !url.startsWith("/")) {
            // Relative path
            QString normalizedBase = baseUrl;
            if (!normalizedBase.endsWith('/')) {
                normalizedBase += '/';
            }
            return normalizedBase + url;
        }
        return url;
    };
    
    // Extract common fields
    map["id"] = item["id"].toString();
    map["type"] = item["type"].toString();
    map["title"] = item["title"].toString();
    map["name"] = item["name"].toString(); // Some items use "name" instead of "title"
    if (map["title"].toString().isEmpty()) {
        map["title"] = map["name"].toString();
    }
    map["description"] = item["description"].toString();
    
    // Resolve poster URL
    QString poster = item["poster"].toString();
    QString resolvedPoster = resolveUrl(poster);
    map["poster"] = resolvedPoster;
    map["posterUrl"] = resolvedPoster; // Alias for QML
    
    // Resolve background/backdrop URL
    QString background = item["background"].toString();
    QString resolvedBackground = resolveUrl(background);
    map["background"] = resolvedBackground;
    map["backdropUrl"] = resolvedBackground; // Alias for QML
    
    // Resolve logo URL
    QString logo = item["logo"].toString();
    QString resolvedLogo = resolveUrl(logo);
    map["logo"] = resolvedLogo;
    map["logoUrl"] = resolvedLogo; // Alias for QML
    
    // Extract IDs
    QVariant idVar = item["id"];
    if (idVar.typeId() == QMetaType::QJsonObject) {
        QJsonObject ids = item["id"].toObject();
        map["imdbId"] = ids["imdb"].toString();
        map["tmdbId"] = ids["tmdb"].toString();
        map["traktId"] = ids["trakt"].toString();
    } else {
        // ID is a string - could be "tmdb:123" or "tt1234567" format
        QString idStr = item["id"].toString();
        map["id"] = idStr;
        
        // Try to extract TMDB ID from "tmdb:123" format
        if (idStr.startsWith("tmdb:")) {
            QString tmdbId = idStr.mid(5); // Remove "tmdb:" prefix
            map["tmdbId"] = tmdbId;
        }
    }
    
    // Extract IMDB ID from separate field if available
    if (item.contains("imdb_id")) {
        QString imdbId = item["imdb_id"].toString();
        map["imdbId"] = imdbId;
        // If ID is not set or is tmdb format, use imdb_id as the main ID
        if (map["id"].toString().isEmpty() || map["id"].toString().startsWith("tmdb:")) {
            if (!imdbId.isEmpty()) {
                map["id"] = imdbId;
            }
        }
    }
    
    // Extract year - check multiple sources
    bool yearFound = false;
    if (item.contains("year")) {
        QVariant yearVar = item["year"];
        if (yearVar.typeId() == QMetaType::Int || yearVar.typeId() == QMetaType::LongLong) {
            map["year"] = yearVar.toInt();
            yearFound = true;
        } else {
            QString yearStr = yearVar.toString();
            bool ok;
            int year = yearStr.toInt(&ok);
            if (ok && year > 1900 && year < 2100) {
                map["year"] = year;
                yearFound = true;
            }
        }
    }
    
    if (!yearFound && item.contains("releaseInfo")) {
        QString releaseInfo = item["releaseInfo"].toString();
        // releaseInfo can be "2025" (just year) or "2023-01-01" (full date) format
        if (releaseInfo.length() >= 4) {
            bool ok;
            int year = releaseInfo.left(4).toInt(&ok);
            if (ok && year > 1900 && year < 2100) {
                map["year"] = year;
                yearFound = true;
            }
        }
    }
    
    if (!yearFound && item.contains("released")) {
        QString released = item["released"].toString();
        // released is ISO date format "2025-11-26T00:00:00.000Z"
        if (released.length() >= 4) {
            bool ok;
            int year = released.left(4).toInt(&ok);
            if (ok && year > 1900 && year < 2100) {
                map["year"] = year;
            }
        }
    }
    
    // Extract rating - check multiple sources
    if (item.contains("imdbRating")) {
        QString rating = item["imdbRating"].toString();
        map["rating"] = rating;
    } else if (item.contains("rating")) {
        QString rating = item["rating"].toString();
        map["rating"] = rating;
    }
    
    // Extract genres
    if (item.contains("genres")) {
        QJsonArray genres = item["genres"].toArray();
        QStringList genreList;
        for (const QJsonValue& genre : genres) {
            genreList.append(genre.toString());
        }
        map["genres"] = genreList;
    }

    // === DATA NORMALIZATION FOR QML COMPATIBILITY ===
    // Ensure ALL expected fields exist with proper types and defaults

    // Required string fields with fallbacks
    if (map["title"].toString().isEmpty()) {
        map["title"] = map["name"].toString();  // Fallback to name
    }
    if (map["title"].toString().isEmpty()) {
        map["title"] = "Unknown Title";  // Final fallback
    }

    if (map["posterUrl"].toString().isEmpty()) {
        map["posterUrl"] = map["poster"].toString();  // Use poster as posterUrl
    }
    if (map["posterUrl"].toString().isEmpty()) {
        map["posterUrl"] = "";  // Explicit empty string
    }

    // Required numeric fields with validation
    if (!map.contains("year") || map["year"].toInt() <= 0) {
        map["year"] = 0;  // Default to 0
    }

    // Required string fields
    if (!map.contains("rating")) {
        map["rating"] = "";  // Default empty
    }

    if (!map.contains("description")) {
        map["description"] = "";  // Default empty
    }

    if (!map.contains("id")) {
        map["id"] = "";  // Default empty
    }

    // Optional fields for ListModel compatibility
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

    if (!map.contains("type")) {
        map["type"] = "";
    }

    // Ensure all URL fields are strings
    if (!map.contains("background")) {
        map["background"] = "";
    }

    if (!map.contains("backdropUrl")) {
        map["backdropUrl"] = "";
    }

    if (!map.contains("logo")) {
        map["logo"] = "";
    }

    if (!map.contains("logoUrl")) {
        map["logoUrl"] = "";
    }

    // ID fields
    if (!map.contains("imdbId")) {
        map["imdbId"] = "";
    }

    if (!map.contains("tmdbId")) {
        map["tmdbId"] = "";
    }

    if (!map.contains("traktId")) {
        map["traktId"] = "";
    }

    return map;
}

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

QVariantMap LibraryService::continueWatchingItemToVariantMap(const QVariantMap& traktItem, const QJsonObject& tmdbData)
{
    QVariantMap map;
    
    // Extract type from Trakt (only for identification)
    QString type = traktItem["type"].toString();
    map["type"] = type;
    
    // Extract progress from Trakt (only Trakt provides watch progress)
    double progress = traktItem["progress"].toDouble();
    map["progress"] = progress;
    map["progressPercent"] = progress;
    
    // Extract episode information from Trakt (only Trakt provides episode-level data)
    if (type == "episode") {
        QVariantMap episode = traktItem["episode"].toMap();
        if (!episode.isEmpty()) {
            map["season"] = episode["season"].toInt();
            map["episode"] = episode["number"].toInt();
            map["episodeTitle"] = episode["title"].toString(); // Episode title from Trakt
            qDebug() << "[LibraryService] Extracted episode data - S" << map["season"].toInt() 
                     << "E" << map["episode"].toInt() << ":" << map["episodeTitle"].toString();
        } else {
            qWarning() << "[LibraryService] Episode type but no episode data in Trakt item!";
            // Set defaults to avoid missing data
            map["season"] = 0;
            map["episode"] = 0;
            map["episodeTitle"] = "";
        }
    }
    
    // Extract IMDB ID from Trakt (needed for identification, but display data comes from TMDB)
    QString imdbId;
    if (type == "movie") {
        QVariantMap movie = traktItem["movie"].toMap();
        QVariantMap ids = movie["ids"].toMap();
        imdbId = ids["imdb"].toString();
    } else if (type == "episode") {
        QVariantMap show = traktItem["show"].toMap();
        QVariantMap showIds = show["ids"].toMap();
        imdbId = showIds["imdb"].toString();
    }
    map["imdbId"] = imdbId;
    map["id"] = imdbId;
    
    // ALL display data comes from TMDB
    if (type == "movie") {
        // Movie display data from TMDB
        map["title"] = tmdbData["title"].toString();
        
        // Extract year from release date
        QString releaseDate = tmdbData["release_date"].toString();
        if (!releaseDate.isEmpty() && releaseDate.length() >= 4) {
            map["year"] = releaseDate.left(4).toInt();
        }
        
        // Extract images from TMDB
        QString posterUrl = TmdbDataExtractor::extractPosterUrl(tmdbData);
        QString backdropUrl = TmdbDataExtractor::extractBackdropUrl(tmdbData);
        QString logoUrl = TmdbDataExtractor::extractLogoUrl(tmdbData);
        
        // Fallback: backdrop -> poster
        if (backdropUrl.isEmpty() && !posterUrl.isEmpty()) {
            backdropUrl = posterUrl;
        }
        
        map["posterUrl"] = posterUrl;
        map["backdropUrl"] = backdropUrl;
        map["logoUrl"] = logoUrl;
        map["description"] = tmdbData["overview"].toString();
        
    } else if (type == "episode") {
        // TV Show display data from TMDB (show-level, not episode-level)
        map["title"] = tmdbData["name"].toString();
        
        // Extract year from first air date
        QString firstAirDate = tmdbData["first_air_date"].toString();
        if (!firstAirDate.isEmpty() && firstAirDate.length() >= 4) {
            map["year"] = firstAirDate.left(4).toInt();
        }
        
        // Extract images from TMDB
        QString posterUrl = TmdbDataExtractor::extractPosterUrl(tmdbData);
        QString backdropUrl = TmdbDataExtractor::extractBackdropUrl(tmdbData);
        QString logoUrl = TmdbDataExtractor::extractLogoUrl(tmdbData);
        
        // Fallback: backdrop -> poster
        if (backdropUrl.isEmpty() && !posterUrl.isEmpty()) {
            backdropUrl = posterUrl;
        }
        
        map["posterUrl"] = posterUrl;
        map["backdropUrl"] = backdropUrl;
        map["logoUrl"] = logoUrl;
        map["description"] = tmdbData["overview"].toString();
    }
    
    // Extract watched_at from Trakt (metadata, not display)
    map["watchedAt"] = traktItem["paused_at"].toString();
    
    // === DATA NORMALIZATION FOR QML COMPATIBILITY ===
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
    
    if (!map.contains("progress")) {
        map["progress"] = 0.0;
    }
    
    if (!map.contains("progressPercent")) {
        map["progressPercent"] = 0.0;
    }
    
    if (!map.contains("imdbId")) {
        map["imdbId"] = "";
    }
    
    if (!map.contains("description")) {
        map["description"] = "";
    }
    
    return map;
}

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
            QVariantMap item = catalogItemToVariantMap(value.toObject(), baseUrl);
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
    
    // Check if this is a details request
    if (!m_pendingDetailsContentId.isEmpty() && m_pendingDetailsContentId == imdbId) {
        onTmdbIdFoundForDetails(imdbId, tmdbId);
        return;
    }
    
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
    
    // Check if this is a details request
    if (!m_pendingDetailsContentId.isEmpty() && m_tmdbIdToImdbIdForDetails.contains(tmdbId)) {
        onTmdbMovieDetailsFetched(tmdbId, data);
        return;
    }
    
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
    QVariantMap continueItem = continueWatchingItemToVariantMap(traktItem, data);
    
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
    
    // Check if this is a details request
    if (!m_pendingDetailsContentId.isEmpty() && m_tmdbIdToImdbIdForDetails.contains(tmdbId)) {
        onTmdbTvDetailsFetched(tmdbId, data);
        return;
    }
    
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
    
    QVariantMap continueItem = continueWatchingItemToVariantMap(traktItem, data);
    
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
    m_tmdbIdToImdbIdForDetails.clear();
    
    // Try to extract TMDB ID
    int tmdbId = IdParser::extractTmdbId(contentId);
    
    // If IMDB ID, search for TMDB ID first
    if (tmdbId == 0 && contentId.startsWith("tt")) {
        qDebug() << "[LibraryService] IMDB ID detected, fetching TMDB ID first";
        m_tmdbService->getTmdbIdFromImdb(contentId);
        return;
    }
    
    if (tmdbId == 0) {
        emit error("Could not extract TMDB ID from content ID");
        return;
    }
    
    // Store TMDB ID mapping for direct TMDB IDs
    m_tmdbIdToImdbIdForDetails[tmdbId] = contentId;
    
    // Fetch metadata based on type
    if (type == "movie") {
        m_tmdbService->getMovieMetadata(tmdbId);
    } else if (type == "series" || type == "tv") {
        m_tmdbService->getTvMetadata(tmdbId);
    } else {
        emit error(QString("Unknown content type: %1").arg(type));
    }
}

void LibraryService::onTmdbIdFoundForDetails(const QString& imdbId, int tmdbId)
{
    qDebug() << "[LibraryService] TMDB ID found for details - IMDB:" << imdbId << "TMDB:" << tmdbId;
    
    m_tmdbIdToImdbIdForDetails[tmdbId] = imdbId;
    
    // Fetch metadata based on type
    if (m_pendingDetailsType == "movie") {
        m_tmdbService->getMovieMetadata(tmdbId);
    } else if (m_pendingDetailsType == "series" || m_pendingDetailsType == "tv") {
        m_tmdbService->getTvMetadata(tmdbId);
    }
}

void LibraryService::onTmdbMovieDetailsFetched(int tmdbId, const QJsonObject& data)
{
    qDebug() << "[LibraryService] TMDB movie details fetched for TMDB ID:" << tmdbId;
    
    QString imdbId = m_tmdbIdToImdbIdForDetails.value(tmdbId, m_pendingDetailsContentId);
    QVariantMap details = tmdbDataToDetailVariantMap(data, imdbId, "movie");
    
    if (!details.isEmpty()) {
        emit itemDetailsLoaded(details);
    } else {
        emit error("Failed to convert TMDB movie data to detail map");
    }
}

void LibraryService::onTmdbTvDetailsFetched(int tmdbId, const QJsonObject& data)
{
    qDebug() << "[LibraryService] TMDB TV details fetched for TMDB ID:" << tmdbId;
    
    QString imdbId = m_tmdbIdToImdbIdForDetails.value(tmdbId, m_pendingDetailsContentId);
    QVariantMap details = tmdbDataToDetailVariantMap(data, imdbId, "series");
    
    if (!details.isEmpty()) {
        emit itemDetailsLoaded(details);
    } else {
        emit error("Failed to convert TMDB TV data to detail map");
    }
}

QString LibraryService::formatDateDDMMYYYY(const QString& dateString)
{
    if (dateString.isEmpty()) {
        return QString();
    }
    
    // Expected format: YYYY-MM-DD
    QStringList parts = dateString.split("-");
    if (parts.size() >= 3) {
        return QString("%1-%2-%3").arg(parts[2], parts[1], parts[0]);
    }
    
    return dateString; // Return as-is if format is unexpected
}

QVariantMap LibraryService::tmdbDataToDetailVariantMap(const QJsonObject& tmdbData, const QString& contentId, const QString& type)
{
    QVariantMap map;
    
    try {
        // Basic info
        map["id"] = contentId;
        map["type"] = (type == "movie") ? "movie" : "series";
        
        // Title/Name
        if (type == "movie") {
            map["title"] = tmdbData["title"].toString();
            map["name"] = tmdbData["title"].toString();
        } else {
            map["title"] = tmdbData["name"].toString();
            map["name"] = tmdbData["name"].toString();
        }
        
        // Images
        map["backdropUrl"] = TmdbDataExtractor::extractBackdropUrl(tmdbData);
        map["logoUrl"] = TmdbDataExtractor::extractLogoUrl(tmdbData);
        map["posterUrl"] = TmdbDataExtractor::extractPosterUrl(tmdbData);
        
        // Description
        map["description"] = tmdbData["overview"].toString();
        map["overview"] = tmdbData["overview"].toString();
        
        // Release dates
        if (type == "movie") {
            QString releaseDate = tmdbData["release_date"].toString();
            map["releaseDate"] = formatDateDDMMYYYY(releaseDate);
            map["firstAirDate"] = formatDateDDMMYYYY(releaseDate);
            
            // Extract year
            if (!releaseDate.isEmpty() && releaseDate.length() >= 4) {
                map["year"] = releaseDate.left(4).toInt();
            }
        } else {
            QString firstAirDate = tmdbData["first_air_date"].toString();
            map["firstAirDate"] = formatDateDDMMYYYY(firstAirDate);
            map["releaseDate"] = formatDateDDMMYYYY(firstAirDate);
            
            // Extract year
            if (!firstAirDate.isEmpty() && firstAirDate.length() >= 4) {
                map["year"] = firstAirDate.left(4).toInt();
            }
            
            // Number of seasons
            map["numberOfSeasons"] = tmdbData["number_of_seasons"].toInt();
        }
        
        // Content rating
        QString maturityRating = TmdbDataExtractor::extractMaturityRating(tmdbData, type);
        map["contentRating"] = maturityRating;
        
        // Genres
        QVariantList genres;
        QJsonArray genresArray = tmdbData["genres"].toArray();
        for (const QJsonValue& value : genresArray) {
            QJsonObject genre = value.toObject();
            genres.append(genre["name"].toString());
        }
        map["genres"] = genres;
        
        // Ratings
        double voteAverage = tmdbData["vote_average"].toDouble();
        map["tmdbRating"] = QString::number(voteAverage, 'f', 1);
        
        // IMDB rating from external_ids
        QString imdbRating;
        QJsonObject externalIds = tmdbData["external_ids"].toObject();
        if (!externalIds.isEmpty()) {
            // Note: TMDB doesn't provide IMDB rating directly, we'd need another API
            // For now, we can use TMDB rating as fallback or leave empty
            imdbRating = ""; // Placeholder - would need separate IMDB API call
        }
        map["imdbRating"] = imdbRating;
        map["rtRating"] = ""; // Placeholder
        map["mcRating"] = ""; // Placeholder
        
        // IDs
        map["imdbId"] = externalIds["imdb_id"].toString();
        map["tmdbId"] = QString::number(tmdbData["id"].toInt());
        
        // Cast and crew
        QJsonObject castAndCrew = TmdbDataExtractor::extractCastAndCrew(tmdbData);
        QJsonArray cast = castAndCrew["cast"].toArray();
        QJsonArray crew = castAndCrew["crew"].toArray();
        
        QVariantList castList;
        QVariantList crewList;
        
        // Build full profile image URLs for cast
        QString imageBaseUrl = Configuration::instance().tmdbImageBaseUrl();
        for (const QJsonValue& value : cast) {
            QJsonObject person = value.toObject();
            QVariantMap personMap = person.toVariantMap();
            
            // Build full profile image URL
            QString profilePath = person["profile_path"].toString();
            if (!profilePath.isEmpty() && !profilePath.startsWith("http")) {
                personMap["profileImageUrl"] = imageBaseUrl + "w185" + profilePath;
            } else if (profilePath.startsWith("http")) {
                personMap["profileImageUrl"] = profilePath;
            } else {
                personMap["profileImageUrl"] = "";
            }
            
            castList.append(personMap);
        }
        for (const QJsonValue& value : crew) {
            crewList.append(value.toObject().toVariantMap());
        }
        
        map["castFull"] = castList;
        map["crewFull"] = crewList;
        
        // Videos/Trailers
        QVariantList videos;
        QJsonObject videosObj = tmdbData["videos"].toObject();
        QJsonArray videoResults = videosObj["results"].toArray();
        for (const QJsonValue& value : videoResults) {
            QJsonObject video = value.toObject();
            if (video["type"].toString() == "Trailer" && video["site"].toString() == "YouTube") {
                QVariantMap trailer;
                trailer["name"] = video["name"].toString();
                trailer["key"] = video["key"].toString();
                trailer["site"] = video["site"].toString();
                videos.append(trailer);
            }
        }
        map["videos"] = videos;
        
    } catch (...) {
        qDebug() << "Error converting TMDB data to detail variant map";
        return QVariantMap();
    }
    
    return map;
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
    
    QVariantList items;
    Configuration& config = Configuration::instance();
    
    for (const QJsonValue& value : results) {
        if (!value.isObject()) continue;
        
        QJsonObject movie = value.toObject();
        QVariantMap item;
        
        // Extract basic info
        int tmdbId = movie["id"].toInt();
        item["id"] = QString("tmdb:%1").arg(tmdbId);
        item["tmdbId"] = QString::number(tmdbId);  // Store raw TMDB ID for direct use
        item["type"] = "movie";
        item["title"] = movie["title"].toString();
        item["name"] = movie["title"].toString();
        
        // Extract year from release_date
        QString releaseDate = movie["release_date"].toString();
        if (!releaseDate.isEmpty() && releaseDate.length() >= 4) {
            item["year"] = releaseDate.left(4).toInt();
        }
        
        // Poster URL
        QString posterPath = movie["poster_path"].toString();
        if (!posterPath.isEmpty()) {
            item["posterUrl"] = config.tmdbImageBaseUrl() + "w500" + posterPath;
        }
        
        // Rating
        double voteAverage = movie["vote_average"].toDouble();
        if (voteAverage > 0) {
            item["rating"] = QString::number(voteAverage, 'f', 1);
        }
        
        items.append(item);
    }
    
    emit similarItemsLoaded(items);
}

void LibraryService::onSimilarTvFetched(int tmdbId, const QJsonArray& results)
{
    qDebug() << "[LibraryService] Similar TV shows fetched for TMDB ID:" << tmdbId << "count:" << results.size();
    
    QVariantList items;
    Configuration& config = Configuration::instance();
    
    for (const QJsonValue& value : results) {
        if (!value.isObject()) continue;
        
        QJsonObject tv = value.toObject();
        QVariantMap item;
        
        // Extract basic info
        int tmdbId = tv["id"].toInt();
        item["id"] = QString("tmdb:%1").arg(tmdbId);
        item["tmdbId"] = QString::number(tmdbId);  // Store raw TMDB ID for direct use
        item["type"] = "series";
        item["title"] = tv["name"].toString();
        item["name"] = tv["name"].toString();
        
        // Extract year from first_air_date
        QString firstAirDate = tv["first_air_date"].toString();
        if (!firstAirDate.isEmpty() && firstAirDate.length() >= 4) {
            item["year"] = firstAirDate.left(4).toInt();
        }
        
        // Poster URL
        QString posterPath = tv["poster_path"].toString();
        if (!posterPath.isEmpty()) {
            item["posterUrl"] = config.tmdbImageBaseUrl() + "w500" + posterPath;
        }
        
        // Rating
        double voteAverage = tv["vote_average"].toDouble();
        if (voteAverage > 0) {
            item["rating"] = QString::number(voteAverage, 'f', 1);
        }
        
        items.append(item);
    }
    
    emit similarItemsLoaded(items);
}

