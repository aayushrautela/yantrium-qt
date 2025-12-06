#include "library_service.h"
#include "core/database/database_manager.h"
#include "core/services/id_parser.h"
#include "core/services/configuration.h"
#include "core/services/frontend_data_mapper.h"
#include "core/services/local_library_service.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

LibraryService::LibraryService(
    std::shared_ptr<AddonRepository> addonRepository,
    std::shared_ptr<TmdbDataService> tmdbService,
    std::shared_ptr<TmdbSearchService> tmdbSearchService,
    std::shared_ptr<MediaMetadataService> mediaMetadataService,
    std::shared_ptr<OmdbService> omdbService,
    std::shared_ptr<LocalLibraryService> localLibraryService,
    std::unique_ptr<CatalogPreferencesDao> catalogPreferencesDao,
    TraktCoreService* traktService,
    QObject* parent)
    : QObject(parent)
    , m_addonRepository(std::move(addonRepository))
    , m_traktService(traktService ? traktService : &TraktCoreService::instance())
    , m_tmdbService(std::move(tmdbService))
    , m_tmdbSearchService(std::move(tmdbSearchService))
    , m_mediaMetadataService(std::move(mediaMetadataService))
    , m_omdbService(std::move(omdbService))
    , m_localLibraryService(std::move(localLibraryService))
    , m_catalogPreferencesDao(std::move(catalogPreferencesDao))
    , m_pendingCatalogRequests(0)
    , m_isLoadingCatalogs(false)
    , m_isRawExport(false)
    , m_pendingHeroRequests(0)
    , m_isLoadingHeroItems(false)
    , m_pendingHeroTmdbRequests(0)
    , m_pendingTmdbRequests(0)
    , m_searchMoviesReceived(false)
    , m_searchTvReceived(false)
{
    // Connect to Trakt service for continue watching
    connect(m_traktService, &TraktCoreService::playbackProgressFetched,
            this, &LibraryService::onPlaybackProgressFetched);
    
    // Connect to TMDB service for continue watching images
    if (m_tmdbService) {
        connect(m_tmdbService.get(), &TmdbDataService::movieMetadataFetched,
                this, &LibraryService::onTmdbMovieMetadataFetched);
        connect(m_tmdbService.get(), &TmdbDataService::tvMetadataFetched,
                this, &LibraryService::onTmdbTvMetadataFetched);
        connect(m_tmdbService.get(), &TmdbDataService::tmdbIdFound,
                this, &LibraryService::onTmdbIdFound);
        connect(m_tmdbService.get(), &TmdbDataService::error,
                this, &LibraryService::onTmdbError);
        connect(m_tmdbService.get(), &TmdbDataService::similarMoviesFetched,
                this, &LibraryService::onSimilarMoviesFetched);
        connect(m_tmdbService.get(), &TmdbDataService::similarTvFetched,
                this, &LibraryService::onSimilarTvFetched);
        connect(m_tmdbService.get(), &TmdbDataService::tvSeasonDetailsFetched,
                this, &LibraryService::onTvSeasonDetailsFetched);
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
        qDebug() << "[LibraryService] Processing addon:" << addon.id << "(" << addon.name << ")";
        
        AddonManifest manifest = m_addonRepository->getManifest(addon);
        if (manifest.id().isEmpty()) {
            qDebug() << "[LibraryService] ERROR: Could not get manifest for addon:" << addon.id;
            continue;
        }
        
            qDebug() << "[LibraryService] Got manifest for addon:" << addon.id;
        
        // Get catalogs from manifest
        QList<CatalogDefinition> catalogs = manifest.catalogs();
        qDebug() << "[LibraryService] Found" << catalogs.size() << "catalog definitions in manifest for addon:" << addon.id;
        
        if (catalogs.isEmpty()) {
            qDebug() << "[LibraryService] WARNING: No catalogs found for addon:" << addon.id;
            continue;
        }
        
        // Get base URL for this addon
        QString baseUrl = AddonClient::extractBaseUrl(addon.manifestUrl);
        qDebug() << "[LibraryService] Extracted base URL:" << baseUrl << "for addon:" << addon.id;
        
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
            auto preference = m_catalogPreferencesDao->getPreference(addon.id, catalogType, catalogId);
            bool isEnabled = preference ? preference->enabled : true;
            
            qDebug() << "[LibraryService] Catalog preference check - enabled:" << isEnabled 
                     << "(preference exists:" << (preference != nullptr) << ")";
            
            if (!isEnabled) {
                disabledCatalogCount++;
                qDebug() << "[LibraryService] SKIPPING disabled catalog:" << catalogName << "(" << catalogType << "," << catalogId << ") from addon:" << addon.id;
                continue;
            }
            
            enabledCatalogCount++;
            m_pendingCatalogRequests++;
            qDebug() << "[LibraryService] FETCHING catalog:" << catalogName << "(" << catalogType << "," << catalogId << ") from addon:" << addon.id;
            qDebug() << "[LibraryService] Pending requests now:" << m_pendingCatalogRequests;
            
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
            
            qDebug() << "[LibraryService] Created new client and calling getCatalog(" << catalogType << "," << catalogId << ")";
            client->getCatalog(catalogType, catalogId);
        }
        
        qDebug() << "[LibraryService] Addon" << addon.id << "- Enabled catalogs:" << enabledCatalogCount << "Disabled catalogs:" << disabledCatalogCount;
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
    if (addon.id.isEmpty()) {
        emit error(QString("Addon not found: %1").arg(addonId));
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
            if (addon.id.isEmpty()) {
                qWarning() << "[LibraryService] Hero catalog addon not found:" << heroCatalog.addonId;
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

void LibraryService::searchTmdb(const QString& query)
{
    qWarning() << "[LibraryService] ===== searchTmdb CALLED =====";
    qWarning() << "[LibraryService] Query:" << query;
    
    if (query.trimmed().isEmpty()) {
        qWarning() << "[LibraryService] Empty query, emitting empty results";
        emit tmdbSearchResultsLoaded(QVariantList());
        return;
    }
    
    if (!m_tmdbSearchService) {
        qWarning() << "[LibraryService] ERROR: m_tmdbSearchService is null!";
        emit tmdbSearchResultsLoaded(QVariantList());
        return;
    }
    
    qWarning() << "[LibraryService] Searching TMDB for:" << query;
    qWarning() << "[LibraryService] m_tmdbSearchService pointer:" << (void*)m_tmdbSearchService.get();
    
    // Track pending searches
    m_pendingSearchQuery = query;
    m_pendingSearchMovies.clear();
    m_pendingSearchTv.clear();
    m_searchMoviesReceived = false;
    m_searchTvReceived = false;
    
    qWarning() << "[LibraryService] Disconnecting previous signal connections";
    // Connect to search service signals (use disconnect first to avoid duplicates)
    disconnect(m_tmdbSearchService.get(), &TmdbSearchService::moviesFound, this, nullptr);
    disconnect(m_tmdbSearchService.get(), &TmdbSearchService::tvFound, this, nullptr);
    disconnect(m_tmdbSearchService.get(), &TmdbSearchService::error, this, nullptr);
    
    qWarning() << "[LibraryService] Connecting to search service signals";
    bool moviesConnected = connect(m_tmdbSearchService.get(), &TmdbSearchService::moviesFound, this, &LibraryService::onSearchMoviesFound);
    bool tvConnected = connect(m_tmdbSearchService.get(), &TmdbSearchService::tvFound, this, &LibraryService::onSearchTvFound);
    bool errorConnected = connect(m_tmdbSearchService.get(), &TmdbSearchService::error, this, &LibraryService::onSearchError);
    
    qWarning() << "[LibraryService] Signal connections - movies:" << moviesConnected << "tv:" << tvConnected << "error:" << errorConnected;
    
    // Search both movies and TV
    qWarning() << "[LibraryService] Calling searchMovies and searchTv";
    m_tmdbSearchService->searchMovies(query);
    m_tmdbSearchService->searchTv(query);
    qWarning() << "[LibraryService] Search requests sent";
}

void LibraryService::onSearchMoviesFound(const QVariantList& results)
{
    qWarning() << "[LibraryService] ===== onSearchMoviesFound CALLED =====";
    qWarning() << "[LibraryService] Movies found:" << results.size() << "results";
    
    if (results.size() > 0) {
        qWarning() << "[LibraryService] First movie sample:" << results.first().toMap();
    }
    
    m_pendingSearchMovies = results;
    m_searchMoviesReceived = true;
    qWarning() << "[LibraryService] Movies received flag set, calling finishSearch";
    finishSearch();
}

void LibraryService::onSearchTvFound(const QVariantList& results)
{
    qWarning() << "[LibraryService] ===== onSearchTvFound CALLED =====";
    qWarning() << "[LibraryService] TV found:" << results.size() << "results";
    
    if (results.size() > 0) {
        qWarning() << "[LibraryService] First TV show sample:" << results.first().toMap();
    }
    
    m_pendingSearchTv = results;
    m_searchTvReceived = true;
    qWarning() << "[LibraryService] TV received flag set, calling finishSearch";
    finishSearch();
}

void LibraryService::onSearchError(const QString& message)
{
    qWarning() << "[LibraryService] ===== onSearchError CALLED =====";
    qWarning() << "[LibraryService] Search error:" << message;
    // Still try to finish with what we have
    finishSearch();
}

void LibraryService::finishSearch()
{
    qWarning() << "[LibraryService] ===== finishSearch CALLED =====";
    qWarning() << "[LibraryService] Movies received:" << m_searchMoviesReceived << "TV received:" << m_searchTvReceived;
    
    // Wait for both searches to complete
    if (!m_searchMoviesReceived || !m_searchTvReceived) {
        qWarning() << "[LibraryService] Waiting for both searches to complete...";
        return;
    }
    
    qWarning() << "[LibraryService] Both searches complete, combining results";
    qWarning() << "[LibraryService] Movies count:" << m_pendingSearchMovies.size() << "TV count:" << m_pendingSearchTv.size();
    
    // Combine results and format for frontend
    QVariantList combinedResults;
    
    // Add movies with type
    for (const QVariant& item : m_pendingSearchMovies) {
        QVariantMap movie = item.toMap();
        movie["type"] = "movie";
        // Extract year from releaseDate
        QString releaseDate = movie["releaseDate"].toString();
        if (!releaseDate.isEmpty() && releaseDate.length() >= 4) {
            movie["year"] = releaseDate.left(4).toInt();
        }
        // Use title for both title and name
        if (movie["title"].toString().isEmpty() && !movie["name"].toString().isEmpty()) {
            movie["title"] = movie["name"];
        }
        // Ensure we have contentId and tmdbId
        int id = movie["id"].toInt();
        if (id > 0) {
            movie["tmdbId"] = QString::number(id);
            movie["contentId"] = "tmdb:" + QString::number(id);
        }
        // Build poster URL if we have posterPath
        if (movie.contains("posterPath") && !movie["posterPath"].toString().isEmpty()) {
            QString posterPath = movie["posterPath"].toString();
            if (!posterPath.startsWith("http")) {
                movie["posterUrl"] = "https://image.tmdb.org/t/p/w500" + posterPath;
            } else {
                movie["posterUrl"] = posterPath;
            }
        }
        // Format rating
        if (movie.contains("voteAverage")) {
            double rating = movie["voteAverage"].toDouble();
            movie["rating"] = QString::number(rating, 'f', 1);
        }
        combinedResults.append(movie);
    }
    
    // Add TV shows with type
    for (const QVariant& item : m_pendingSearchTv) {
        QVariantMap tv = item.toMap();
        tv["type"] = "tv";
        // Extract year from firstAirDate
        QString firstAirDate = tv["firstAirDate"].toString();
        if (!firstAirDate.isEmpty() && firstAirDate.length() >= 4) {
            tv["year"] = firstAirDate.left(4).toInt();
        }
        // Use name for title
        if (tv["title"].toString().isEmpty() && !tv["name"].toString().isEmpty()) {
            tv["title"] = tv["name"];
        }
        // Ensure we have contentId and tmdbId
        int id = tv["id"].toInt();
        if (id > 0) {
            tv["tmdbId"] = QString::number(id);
            tv["contentId"] = "tmdb:" + QString::number(id);
        }
        // Build poster URL if we have posterPath
        if (tv.contains("posterPath") && !tv["posterPath"].toString().isEmpty()) {
            QString posterPath = tv["posterPath"].toString();
            if (!posterPath.startsWith("http")) {
                tv["posterUrl"] = "https://image.tmdb.org/t/p/w500" + posterPath;
            } else {
                tv["posterUrl"] = posterPath;
            }
        }
        // Format rating
        if (tv.contains("voteAverage")) {
            double rating = tv["voteAverage"].toDouble();
            tv["rating"] = QString::number(rating, 'f', 1);
        }
        combinedResults.append(tv);
    }
    
    qWarning() << "[LibraryService] ===== SEARCH COMPLETE =====";
    qWarning() << "[LibraryService] Total combined results:" << combinedResults.size();
    
    if (combinedResults.size() > 0) {
        qWarning() << "[LibraryService] First result sample:" << combinedResults.first().toMap();
    }
    
    qWarning() << "[LibraryService] Emitting tmdbSearchResultsLoaded signal with" << combinedResults.size() << "results";
    emit tmdbSearchResultsLoaded(combinedResults);
    qWarning() << "[LibraryService] Signal emitted";
    
    // Reset state
    m_pendingSearchQuery = "";
    m_pendingSearchMovies.clear();
    m_pendingSearchTv.clear();
    m_searchMoviesReceived = false;
    m_searchTvReceived = false;
    qWarning() << "[LibraryService] Search state reset";
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
    QString baseUrl = AddonClient::extractBaseUrl(addon.manifestUrl);
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
    
    QString addonId = senderClient->property("addonId").toString();

    qCritical() << "[LibraryService] ========== HERO CATALOG FETCHED ==========";
    qCritical() << "[LibraryService] Hero catalog fetched from" << addonId << ", type:" << type << ", items:" << metas.size() << "taking" << itemsPerCatalog;
    qCritical() << "[LibraryService] Current hero items count:" << m_heroItems.size() << ", pending requests:" << m_pendingHeroRequests;
    
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
    
    // Enrich when we have enough items OR all requests are complete
    if (m_pendingHeroRequests == 0 || m_heroItems.size() >= 10) {
        QVariantList itemsToEmit = m_heroItems.mid(0, 10); // Limit to 10 items
        qCritical() << "[LibraryService] ========== STARTING HERO TMDB ENRICHMENT ==========";
        qCritical() << "[LibraryService] Trigger condition: pendingRequests=" << m_pendingHeroRequests << ", totalItems=" << m_heroItems.size();
        qCritical() << "[LibraryService] Starting TMDB enrichment for" << itemsToEmit.size() << "hero items";

        // Enrich hero items with TMDB data
        enrichHeroItemsWithTmdbData(itemsToEmit);
    }
}

void LibraryService::enrichHeroItemsWithTmdbData(const QVariantList& heroItems)
{
    qCritical() << "[LibraryService] ========== HERO ENRICHMENT START ==========";
    qCritical() << "[LibraryService] Enriching" << heroItems.size() << "hero items with TMDB data";

    if (heroItems.isEmpty()) {
        qCritical() << "[LibraryService] No hero items to enrich, emitting empty list";
        m_isLoadingHeroItems = false;
        emit heroItemsLoaded(QVariantList());
        return;
    }

    // Store the items we need to enrich
    m_pendingHeroTmdbItems = heroItems;
    m_pendingHeroTmdbRequests = 0;

    qCritical() << "[LibraryService] Stored" << m_pendingHeroTmdbItems.size() << "hero items for enrichment";

    // Process each hero item
    for (int i = 0; i < heroItems.size(); ++i) {
        const QVariant& itemVar = heroItems[i];
        QVariantMap item = itemVar.toMap();

        qCritical() << "[LibraryService] Processing hero item" << i << ":" << item["title"].toString()
                    << "Type:" << item["type"].toString()
                    << "TMDB ID:" << item["tmdbId"].toString()
                    << "IMDB ID:" << item["imdbId"].toString();

        // Extract TMDB ID
        int tmdbId = 0;
        QString tmdbIdStr = item["tmdbId"].toString();
        if (!tmdbIdStr.isEmpty()) {
            tmdbId = tmdbIdStr.toInt();
            qWarning() << "[LibraryService] Found TMDB ID:" << tmdbId << "for item:" << item["title"].toString();
        }

        // If no TMDB ID, check if we have IMDB ID to convert
        if (tmdbId == 0) {
            QString imdbId = item["imdbId"].toString();
            if (!imdbId.isEmpty()) {
                qWarning() << "[LibraryService] No TMDB ID for hero item, trying IMDB lookup:" << imdbId
                           << "for item:" << item["title"].toString();
                m_pendingHeroTmdbRequests++;
                m_tmdbService->getTmdbIdFromImdb(imdbId);
                continue;
            } else {
                qWarning() << "[LibraryService] No TMDB or IMDB ID for hero item:" << item["title"].toString()
                           << "- SKIPPING";
                continue;
            }
        }

        // We have TMDB ID, fetch metadata
        QString type = item["type"].toString();
        qWarning() << "[LibraryService] Fetching TMDB metadata for" << type << "ID:" << tmdbId
                   << "Title:" << item["title"].toString();

        if (type == "movie") {
            m_pendingHeroTmdbRequests++;
            m_tmdbService->getMovieMetadata(tmdbId);
        } else if (type == "tv" || type == "show") {
            m_pendingHeroTmdbRequests++;
            m_tmdbService->getTvMetadata(tmdbId);
        } else {
            qWarning() << "[LibraryService] Unknown type for TMDB fetch:" << type << "- SKIPPING";
        }
    }

    qWarning() << "[LibraryService] Total TMDB requests initiated:" << m_pendingHeroTmdbRequests;

    // If no requests were made, emit immediately
    if (m_pendingHeroTmdbRequests == 0) {
        qWarning() << "[LibraryService] No TMDB requests needed, emitting hero items immediately";
        m_isLoadingHeroItems = false;
        emit heroItemsLoaded(heroItems);
    } else {
        qWarning() << "[LibraryService] Waiting for" << m_pendingHeroTmdbRequests << "TMDB requests to complete";
    }
}

bool LibraryService::updateHeroItemWithTmdbData(int tmdbId, const QJsonObject& data, const QString& type)
{
    qWarning() << "[LibraryService] Processing TMDB response for hero item - TMDB ID:" << tmdbId << "Type:" << type;

    // Find the hero item with this TMDB ID
    for (int i = 0; i < m_pendingHeroTmdbItems.size(); ++i) {
        QVariantMap heroItem = m_pendingHeroTmdbItems[i].toMap();
        QString itemTmdbIdStr = heroItem["tmdbId"].toString();
        int itemTmdbId = itemTmdbIdStr.toInt();

        if (itemTmdbId == tmdbId) {
            qWarning() << "[LibraryService] Found matching hero item for TMDB ID:" << tmdbId
                       << "Title:" << heroItem["title"].toString();

            // Enrich the hero item with TMDB data
            QVariantMap enrichedItem = FrontendDataMapper::enrichItemWithTmdbData(heroItem, data, type);

            // Log what enrichment added
            QStringList addedFields;
            if (enrichedItem.contains("runtimeFormatted")) addedFields << "runtime";
            if (enrichedItem.contains("genres")) addedFields << "genres";
            if (enrichedItem.contains("badgeText")) addedFields << "badge";

            qWarning() << "[LibraryService] Enriched hero item with:" << addedFields.join(", ")
                       << "- Badge:" << enrichedItem["badgeText"].toString()
                       << "- Runtime:" << enrichedItem["runtimeFormatted"].toString()
                       << "- Genres:" << enrichedItem["genres"].toString();

            // Update the item in our list
            m_pendingHeroTmdbItems[i] = enrichedItem;

            return true;
        }
    }

    qWarning() << "[LibraryService] No matching hero item found for TMDB ID:" << tmdbId;
    return false;
}

bool LibraryService::handleHeroTmdbIdLookup(const QString& imdbId, int tmdbId)
{
    qWarning() << "[LibraryService] TMDB ID lookup result - IMDB:" << imdbId << "-> TMDB:" << tmdbId;

    // Find the hero item with this IMDB ID and update its TMDB ID
    for (int i = 0; i < m_pendingHeroTmdbItems.size(); ++i) {
        QVariantMap heroItem = m_pendingHeroTmdbItems[i].toMap();
        QString itemImdbId = heroItem["imdbId"].toString();

        if (itemImdbId == imdbId) {
            qWarning() << "[LibraryService] Found matching hero item for IMDB ID:" << imdbId
                       << ", setting TMDB ID:" << tmdbId << "for item:" << heroItem["title"].toString();

            // Update the TMDB ID in the hero item
            heroItem["tmdbId"] = QString::number(tmdbId);
            m_pendingHeroTmdbItems[i] = heroItem;

            // Now fetch the metadata for this TMDB ID
            QString type = heroItem["type"].toString();
            qWarning() << "[LibraryService] Now fetching TMDB metadata for" << type << "TMDB ID:" << tmdbId;

            if (type == "movie") {
                m_tmdbService->getMovieMetadata(tmdbId);
            } else if (type == "tv" || type == "show") {
                m_tmdbService->getTvMetadata(tmdbId);
            }

            return true;
        }
    }

    qWarning() << "[LibraryService] No matching hero item found for IMDB ID:" << imdbId;
    return false;
}

void LibraryService::emitHeroItemsWhenReady()
{
    if (m_pendingHeroTmdbRequests == 0) {
        qCritical() << "[LibraryService] ========== HERO ENRICHMENT COMPLETE ==========";
        qCritical() << "[LibraryService] All hero TMDB requests completed, emitting" << m_pendingHeroTmdbItems.size() << "enriched hero items";

        // Log summary of enriched items
        for (int i = 0; i < m_pendingHeroTmdbItems.size(); ++i) {
            QVariantMap item = m_pendingHeroTmdbItems[i].toMap();
            bool hasEnrichment = item["tmdbDataAvailable"].toBool();
            QString badge = item["badgeText"].toString();
            QString runtime = item["runtimeFormatted"].toString();
            QStringList genres = item["genres"].toStringList();

            qWarning() << "[LibraryService] Hero item" << i << ":"
                       << item["title"].toString()
                       << "| Enriched:" << (hasEnrichment ? "YES" : "NO")
                       << "| Badge:" << (badge.isEmpty() ? "none" : badge)
                       << "| Runtime:" << (runtime.isEmpty() ? "none" : runtime)
                       << "| Genres:" << (genres.isEmpty() ? "none" : genres.join(","));
        }

        m_isLoadingHeroItems = false;
        emit heroItemsLoaded(m_pendingHeroTmdbItems);
        qWarning() << "[LibraryService] ========== HERO ITEMS EMITTED ==========";
    } else {
        qWarning() << "[LibraryService] emitHeroItemsWhenReady called but still waiting for"
                   << m_pendingHeroTmdbRequests << "TMDB requests";
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

    // Check if this is for a hero item
    bool isHeroItem = handleHeroTmdbIdLookup(imdbId, tmdbId);
    if (isHeroItem) {
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
    qCritical() << "[LibraryService] TMDB MOVIE metadata fetched for TMDB ID:" << tmdbId
                << "(Title:" << data["title"].toString() << ")";

    // Check if this is for a hero item
    bool isHeroItem = updateHeroItemWithTmdbData(tmdbId, data, "movie");
    if (isHeroItem) {
        qCritical() << "[LibraryService] This was a HERO ITEM - remaining requests:" << (m_pendingHeroTmdbRequests - 1);
        m_pendingHeroTmdbRequests--;
        if (m_pendingHeroTmdbRequests == 0) {
            emitHeroItemsWhenReady();
        }
        return;
    }

    qCritical() << "[LibraryService] This was NOT a hero item, processing as regular request";

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
    qCritical() << "[LibraryService] TMDB TV metadata fetched for TMDB ID:" << tmdbId
                << "(Title:" << data["name"].toString() << ")";

    // Check if this is for a hero item
    bool isHeroItem = updateHeroItemWithTmdbData(tmdbId, data, "tv");
    if (isHeroItem) {
        qCritical() << "[LibraryService] This was a HERO ITEM - remaining requests:" << (m_pendingHeroTmdbRequests - 1);
        m_pendingHeroTmdbRequests--;
        if (m_pendingHeroTmdbRequests == 0) {
            emitHeroItemsWhenReady();
        }
        return;
    }

    qCritical() << "[LibraryService] This was NOT a hero item, processing as regular request";

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
    qWarning() << "[LibraryService] TMDB ERROR:" << message;
    qWarning() << "[LibraryService] Current hero TMDB requests pending:" << m_pendingHeroTmdbRequests;

    // Check if this affects hero items
    if (m_pendingHeroTmdbRequests > 0) {
        qWarning() << "[LibraryService] TMDB error occurred while processing hero items!"
                   << "Remaining hero requests:" << m_pendingHeroTmdbRequests;

        // For hero items, we should still emit what we have after a timeout or error
        // For now, decrement and check if we're done
        m_pendingHeroTmdbRequests--;
        if (m_pendingHeroTmdbRequests == 0) {
            qWarning() << "[LibraryService] All hero TMDB requests failed or completed with errors, emitting what we have";
            emitHeroItemsWhenReady();
        }
    }

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

void LibraryService::onWatchProgressLoaded(const QVariantMap& progress)
{
    QString contentId = progress["contentId"].toString(); // This is now TMDB ID for smart play
    QString type = progress["type"].toString();

    qDebug() << "[LibraryService] onWatchProgressLoaded for TMDB ID" << contentId << "type:" << type;
    qDebug() << "[LibraryService] onWatchProgressLoaded - hasProgress:" << progress["hasProgress"] 
             << "isWatched:" << progress["isWatched"] << "progress:" << progress["progress"];
    qDebug() << "[LibraryService] onWatchProgressLoaded - pendingSmartPlayItems keys:" << m_pendingSmartPlayItems.keys();

    // Check if this was for smart play (contentId is now TMDB ID)
    if (!m_pendingSmartPlayItems.contains(contentId)) {
        qDebug() << "[LibraryService] onWatchProgressLoaded: Not for smart play, ignoring";
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

    qDebug() << "[LibraryService] Processing smart play state - type:" << type 
             << "hasProgress:" << hasProgress << "watchProgress:" << watchProgress
             << "isWatched:" << progress["isWatched"];

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

    qDebug() << "[LibraryService] Smart play state:" << smartPlayState["buttonText"].toString()
             << "action:" << smartPlayState["action"].toString();

    emit smartPlayStateLoaded(smartPlayState);
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

void LibraryService::getSmartPlayState(const QVariantMap& itemData)
{
    qWarning() << "[LibraryService] ===== getSmartPlayState CALLED =====";
    
    // Use TMDB ID as primary identifier
    QString tmdbId = itemData["tmdbId"].toString();
    if (tmdbId.isEmpty()) {
        // Fallback to id if it's a number (likely TMDB ID)
        QString idStr = itemData["id"].toString();
        bool ok;
        idStr.toInt(&ok);
        if (ok) {
            tmdbId = idStr;
        }
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
        qWarning() << "[LibraryService] getSmartPlayState: Unknown type:" << type << ", defaulting to movie";
        type = "movie";
    }
    
    qWarning() << "[LibraryService] getSmartPlayState - tmdbId:" << tmdbId << "type:" << type;
    qWarning() << "[LibraryService] getSmartPlayState - itemData keys:" << itemData.keys();

    if (tmdbId.isEmpty()) {
        qWarning() << "[LibraryService] getSmartPlayState: No TMDB ID found in itemData, cannot check watch history";
        // Emit default state
        QVariantMap defaultState;
        defaultState["buttonText"] = "Play";
        defaultState["action"] = "play";
        defaultState["season"] = -1;
        defaultState["episode"] = -1;
        emit smartPlayStateLoaded(defaultState);
        return;
    }

    // Store the item data for processing when progress is received (use tmdbId as key)
    m_pendingSmartPlayItems[tmdbId] = itemData;
    qWarning() << "[LibraryService] Stored pending item for TMDB ID:" << tmdbId << "pending keys:" << m_pendingSmartPlayItems.keys();

    // Get watch progress from local library service using TMDB ID
    qWarning() << "[LibraryService] Calling getWatchProgressByTmdbId with tmdbId:" << tmdbId << "type:" << type;
    m_localLibraryService->getWatchProgressByTmdbId(tmdbId, type);
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

void LibraryService::loadSeasonEpisodes(int tmdbId, int seasonNumber)
{
    qDebug() << "[LibraryService] Loading episodes for TMDB ID:" << tmdbId << "Season:" << seasonNumber;
    m_tmdbService->getTvSeasonDetails(tmdbId, seasonNumber);
}

void LibraryService::onTvSeasonDetailsFetched(int tmdbId, int seasonNumber, const QJsonObject& data)
{
    qDebug() << "[LibraryService] Season details fetched for TMDB ID:" << tmdbId << "Season:" << seasonNumber;
    
    QVariantList episodes;
    QJsonArray episodesArray = data["episodes"].toArray();
    
    for (const QJsonValue& value : episodesArray) {
        QJsonObject episodeObj = value.toObject();
        QVariantMap episode;
        
        episode["episodeNumber"] = episodeObj["episode_number"].toInt();
        episode["title"] = episodeObj["name"].toString();
        episode["description"] = episodeObj["overview"].toString();
        episode["airDate"] = episodeObj["air_date"].toString();
        
        // Extract runtime (duration in minutes)
        int runtime = episodeObj["runtime"].toInt();
        if (runtime <= 0) {
            // Try to get from show-level runtime if episode doesn't have it
            runtime = data["runtime"].toInt();
        }
        episode["duration"] = runtime;
        
        // Extract thumbnail (still_path)
        QString stillPath = episodeObj["still_path"].toString();
        QString thumbnailUrl = "";
        if (!stillPath.isEmpty()) {
            thumbnailUrl = QString("https://image.tmdb.org/t/p/w500%1").arg(stillPath);
        }
        episode["thumbnailUrl"] = thumbnailUrl;
        
        episodes.append(episode);
    }
    
    qDebug() << "[LibraryService] Mapped" << episodes.size() << "episodes for Season" << seasonNumber;
    emit seasonEpisodesLoaded(seasonNumber, episodes);
}

void LibraryService::clearMetadataCache()
{
    if (m_mediaMetadataService) {
        m_mediaMetadataService->clearMetadataCache();
    }
    if (m_tmdbService) {
        m_tmdbService->clearCache();
    }
    qDebug() << "[LibraryService] All metadata caches cleared";
}

int LibraryService::getMetadataCacheSize() const
{
    int size = 0;
    if (m_mediaMetadataService) {
        size += m_mediaMetadataService->getMetadataCacheSize();
    }
    if (m_tmdbService) {
        size += m_tmdbService->getCacheSize();
    }
    return size;
}

