#include "library_service.h"
#include "core/database/database_manager.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

LibraryService::LibraryService(QObject* parent)
    : QObject(parent)
    , m_addonRepository(new AddonRepository(this))
    , m_traktService(&TraktCoreService::instance())
    , m_catalogPreferencesDao(new CatalogPreferencesDao(DatabaseManager::instance().database()))
    , m_pendingCatalogRequests(0)
    , m_isLoadingCatalogs(false)
{
    // Connect to Trakt service for continue watching
    connect(m_traktService, &TraktCoreService::playbackProgressFetched,
            this, &LibraryService::onPlaybackProgressFetched);
}

LibraryService::~LibraryService()
{
    // Clean up active clients
    qDeleteAll(m_activeClients);
    m_activeClients.clear();
}

void LibraryService::loadCatalogs()
{
    if (m_isLoadingCatalogs) {
        qDebug() << "[LibraryService] Already loading catalogs, skipping";
        return;
    }
    
    m_isLoadingCatalogs = true;
    m_catalogSections.clear();
    m_pendingCatalogRequests = 0;
    
    // Clean up old clients
    qDeleteAll(m_activeClients);
    m_activeClients.clear();
    
    // Get enabled addons
    QList<AddonConfig> enabledAddons = m_addonRepository->getEnabledAddons();
    
    if (enabledAddons.isEmpty()) {
        qDebug() << "[LibraryService] No enabled addons found";
        m_isLoadingCatalogs = false;
        emit catalogsLoaded(QVariantList());
        return;
    }
    
    qDebug() << "[LibraryService] Loading catalogs from" << enabledAddons.size() << "enabled addons";
    
    // For each enabled addon, fetch its catalogs
    for (const AddonConfig& addon : enabledAddons) {
        AddonManifest manifest = m_addonRepository->getManifest(addon);
        if (manifest.id().isEmpty()) {
            qDebug() << "[LibraryService] Could not get manifest for addon:" << addon.id();
            continue;
        }
        
        // Get catalogs from manifest
        QList<CatalogDefinition> catalogs = manifest.catalogs();
        if (catalogs.isEmpty()) {
            qDebug() << "[LibraryService] No catalogs found for addon:" << addon.id();
            continue;
        }
        
        // Create AddonClient for this addon
        QString baseUrl = AddonClient::extractBaseUrl(addon.manifestUrl());
        AddonClient* client = new AddonClient(baseUrl, this);
        m_activeClients.append(client);
        
        // Store addon ID as property for later retrieval
        client->setProperty("addonId", addon.id());
        client->setProperty("baseUrl", baseUrl);
        
        // Connect signals
        connect(client, &AddonClient::catalogFetched, this, &LibraryService::onCatalogFetched);
        connect(client, &AddonClient::error, this, &LibraryService::onClientError);
        
        // Fetch each catalog (only if enabled)
        for (const CatalogDefinition& catalog : catalogs) {
            QString catalogName = catalog.name().isEmpty() ? catalog.type() : catalog.name();
            QString catalogId = catalog.id();
            QString catalogType = catalog.type();
            
            // Check if this catalog is enabled (default to enabled if no preference exists)
            auto preference = m_catalogPreferencesDao->getPreference(addon.id(), catalogType, catalogId);
            bool isEnabled = preference ? preference->enabled : true;
            
            if (!isEnabled) {
                qDebug() << "[LibraryService] Skipping disabled catalog:" << catalogName << "(" << catalogType << "," << catalogId << ") from addon:" << addon.id();
                continue;
            }
            
            m_pendingCatalogRequests++;
            qDebug() << "[LibraryService] Fetching catalog:" << catalogName << "(" << catalogType << "," << catalogId << ") from addon:" << addon.id();
            
            // Store catalog info on client for this specific request
            // We'll use a QMap to track requests
            QString requestKey = QString("%1:%2:%3").arg(addon.id(), catalogType, catalogId);
            client->setProperty(requestKey.toUtf8().constData(), catalogName);
            
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
    
    // Get addon ID from client property
    QString addonId = senderClient->property("addonId").toString();
    if (addonId.isEmpty()) {
        qWarning() << "[LibraryService] Could not find addon ID for catalog";
        if (m_pendingCatalogRequests == 0) {
            finishLoadingCatalogs();
        }
        return;
    }
    
    // Get catalog name from addon manifest
    // Try to find the catalog by matching both type and potentially the response
    AddonConfig addon = m_addonRepository->getAddon(addonId);
    AddonManifest manifest = m_addonRepository->getManifest(addon);
    QString catalogName = type; // Default to type as name
    
    // Find catalog by type (if multiple, use the first one)
    // In a better implementation, we'd track which catalog ID was requested
    for (const CatalogDefinition& catalog : manifest.catalogs()) {
        if (catalog.type() == type) {
            catalogName = catalog.name().isEmpty() ? catalog.type() : catalog.name();
            // If we have multiple catalogs of same type, we could use catalog.id() to distinguish
            // For now, use the first match
            break;
        }
    }
    
    qDebug() << "[LibraryService] Processing catalog:" << catalogName << "type:" << type << "from addon:" << addonId << "items:" << metas.size();
    
    processCatalogData(addonId, catalogName, type, metas);
    
    if (m_pendingCatalogRequests == 0) {
        finishLoadingCatalogs();
    }
}

void LibraryService::onClientError(const QString& errorMessage)
{
    m_pendingCatalogRequests--;
    qWarning() << "[LibraryService] Client error:" << errorMessage;
    
    if (m_pendingCatalogRequests == 0) {
        finishLoadingCatalogs();
    }
}

void LibraryService::onPlaybackProgressFetched(const QVariantList& progress)
{
    m_continueWatching.clear();
    
    for (const QVariant& itemVar : progress) {
        QVariantMap item = itemVar.toMap();
        QVariantMap continueItem = traktPlaybackItemToVariantMap(item);
        if (!continueItem.isEmpty()) {
            m_continueWatching.append(continueItem);
        }
    }
    
    emit continueWatchingLoaded(m_continueWatching);
}

void LibraryService::processCatalogData(const QString& addonId, const QString& catalogName, 
                                       const QString& type, const QJsonArray& metas)
{
    CatalogSection section;
    section.name = catalogName;
    section.type = type;
    section.addonId = addonId;
    
    for (const QJsonValue& value : metas) {
        if (value.isObject()) {
            QVariantMap item = catalogItemToVariantMap(value.toObject());
            if (!item.isEmpty()) {
                section.items.append(item);
            }
        }
    }
    
    if (!section.items.isEmpty()) {
        m_catalogSections.append(section);
        qDebug() << "[LibraryService] Added catalog section:" << catalogName << "with" << section.items.size() << "items";
    }
}

QVariantMap LibraryService::catalogItemToVariantMap(const QJsonObject& item)
{
    QVariantMap map;
    
    // Extract common fields
    map["id"] = item["id"].toString();
    map["type"] = item["type"].toString();
    map["title"] = item["title"].toString();
    map["name"] = item["name"].toString(); // Some items use "name" instead of "title"
    if (map["title"].toString().isEmpty()) {
        map["title"] = map["name"].toString();
    }
    map["description"] = item["description"].toString();
    map["poster"] = item["poster"].toString();
    map["posterUrl"] = item["poster"].toString(); // Alias for QML
    map["background"] = item["background"].toString();
    map["backdropUrl"] = item["background"].toString(); // Alias for QML
    map["logo"] = item["logo"].toString();
    map["logoUrl"] = item["logo"].toString(); // Alias for QML
    
    // Extract IDs
    QVariant idVar = item["id"];
    if (idVar.typeId() == QMetaType::QJsonObject) {
        QJsonObject ids = item["id"].toObject();
        map["imdbId"] = ids["imdb"].toString();
        map["tmdbId"] = ids["tmdb"].toString();
        map["traktId"] = ids["trakt"].toString();
    } else {
        // ID is a string
        QString idStr = item["id"].toString();
        map["imdbId"] = idStr;
        map["id"] = idStr;
    }
    
    // Extract year
    if (item.contains("year")) {
        map["year"] = item["year"].toInt();
    } else if (item.contains("releaseInfo")) {
        QString releaseInfo = item["releaseInfo"].toString();
        // Try to extract year from releaseInfo (e.g., "2023-01-01" -> 2023)
        if (releaseInfo.length() >= 4) {
            bool ok;
            int year = releaseInfo.left(4).toInt(&ok);
            if (ok) {
                map["year"] = year;
            }
        }
    }
    
    // Extract rating
    if (item.contains("imdbRating")) {
        map["rating"] = item["imdbRating"].toString();
    } else if (item.contains("rating")) {
        map["rating"] = item["rating"].toString();
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
        map["posterUrl"] = poster["full"].toString();
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
        map["posterUrl"] = poster["full"].toString();
    }
    
    // Extract watched_at
    map["watchedAt"] = traktItem["paused_at"].toString();
    
    return map;
}

void LibraryService::finishLoadingCatalogs()
{
    m_isLoadingCatalogs = false;
    
    // Convert sections to QVariantList
    QVariantList sections;
    for (const CatalogSection& section : m_catalogSections) {
        QVariantMap sectionMap;
        sectionMap["name"] = section.name;
        sectionMap["type"] = section.type;
        sectionMap["addonId"] = section.addonId;
        sectionMap["items"] = section.items;
        sections.append(sectionMap);
    }
    
    qDebug() << "[LibraryService] Finished loading catalogs. Total sections:" << sections.size();
    emit catalogsLoaded(sections);
}

