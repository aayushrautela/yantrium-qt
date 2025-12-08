#ifndef LIBRARY_SERVICE_H
#define LIBRARY_SERVICE_H

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QList>
#include <memory>
#include <QtQmlIntegration/qqmlintegration.h>
#include "features/addons/logic/addon_repository.h"
#include "features/addons/logic/addon_client.h"
#include "features/addons/models/addon_config.h"
#include "features/addons/models/addon_manifest.h"
#include "core/services/trakt_core_service.h"
#include "core/services/media_metadata_service.h"
#include "core/services/frontend_data_mapper.h"
#include "core/services/omdb_service.h"
#include "core/database/catalog_preferences_dao.h"
#include "interfaces/ilibrary_service.h"

class DatabaseManager;
class LocalLibraryService;

class LibraryService : public QObject, public ILibraryService
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit LibraryService(
        std::shared_ptr<AddonRepository> addonRepository,
        std::shared_ptr<MediaMetadataService> mediaMetadataService,
        std::shared_ptr<OmdbService> omdbService,
        std::shared_ptr<LocalLibraryService> localLibraryService,
        std::unique_ptr<CatalogPreferencesDao> catalogPreferencesDao,
        TraktCoreService* traktService = nullptr,
        QObject* parent = nullptr);
    
    Q_INVOKABLE void loadCatalogs();
    Q_INVOKABLE void loadCatalog(const QString& addonId, const QString& type, const QString& id = QString());
    Q_INVOKABLE void searchCatalogs(const QString& query);
    Q_INVOKABLE void searchTmdb(const QString& query);  // Search TMDB for movies and TV
    Q_INVOKABLE QVariantList getCatalogSections();
    Q_INVOKABLE QVariantList getContinueWatching();
    Q_INVOKABLE void loadCatalogsRaw(); // Load catalogs without processing (for raw export)
    Q_INVOKABLE void loadHeroItems(); // Load items from hero catalogs
    Q_INVOKABLE void loadItemDetails(const QString& contentId, const QString& type, const QString& addonId = QString());
    Q_INVOKABLE void getSmartPlayState(const QVariantMap& itemData);
    void loadSimilarItems(int tmdbId, const QString& type) override;
    void loadSeasonEpisodes(int tmdbId, int seasonNumber) override;

    // Cache management
    Q_INVOKABLE void clearMetadataCache();
    Q_INVOKABLE int getMetadataCacheSize() const;
    
private:
    void loadCatalogsRaw(bool rawMode); // Internal method with raw mode flag
    
signals:
    void catalogsLoaded(const QVariantList& sections);
    void continueWatchingLoaded(const QVariantList& items);
    void searchResultsLoaded(const QVariantList& results);
    void searchSectionLoaded(const QVariantMap& section);  // Single search section loaded incrementally
    void searchSectionsLoaded(const QVariantList& sections);  // Search results as catalog sections (deprecated - use searchSectionLoaded)
    void rawCatalogsLoaded(const QVariantList& rawData); // Raw unprocessed catalog data
    void heroItemsLoaded(const QVariantList& items); // Hero items loaded
    void itemDetailsLoaded(const QVariantMap& details);
    void similarItemsLoaded(const QVariantList& items);
    void smartPlayStateLoaded(const QVariantMap& smartPlayState);
    void seasonEpisodesLoaded(int seasonNumber, const QVariantList& episodes);
    void error(const QString& message);

private slots:
    void onCatalogFetched(const QString& type, const QJsonArray& metas);
    void onClientError(const QString& errorMessage);
    void onPlaybackProgressFetched(const QVariantList& progress);
    void onHeroCatalogFetched(const QString& type, const QJsonArray& metas);
    void onHeroClientError(const QString& errorMessage);
    void onSearchResultsFetched(const QString& type, const QJsonArray& metas);
    void onSearchClientError(const QString& errorMessage);
    void onMediaMetadataLoaded(const QVariantMap& details);
    void onMediaMetadataError(const QString& message);
    void onWatchProgressLoaded(const QVariantMap& progress);

private:
    struct CatalogSection {
        QString name;
        QString type;
        QString addonId;
        QVariantList items;
    };
    
    void processCatalogData(const QString& addonId, const QString& catalogName, const QString& type, const QJsonArray& metas);
    QVariantMap traktPlaybackItemToVariantMap(const QVariantMap& traktItem);
    void finishLoadingCatalogs();
    
    std::shared_ptr<AddonRepository> m_addonRepository;
    TraktCoreService* m_traktService;
    std::shared_ptr<MediaMetadataService> m_mediaMetadataService;
    std::shared_ptr<OmdbService> m_omdbService;
    std::shared_ptr<LocalLibraryService> m_localLibraryService;
    std::unique_ptr<CatalogPreferencesDao> m_catalogPreferencesDao;
    QList<AddonClient*> m_activeClients;
    QList<CatalogSection> m_catalogSections;
    QVariantList m_continueWatching;
    int m_pendingCatalogRequests;
    bool m_isLoadingCatalogs;
    bool m_isRawExport;
    QVariantList m_rawCatalogData;
    
    // Hero items loading
    QVariantList m_heroItems;
    int m_pendingHeroRequests;
    bool m_isLoadingHeroItems;
    void emitHeroItemsWhenReady();
    
    // Continue watching loading
    QMap<QString, QVariantMap> m_pendingContinueWatchingItems; // Content ID -> Trakt item data
    QMap<QString, QVariantMap> m_pendingSmartPlayItems; // Content ID -> item data for smart play
    int m_pendingContinueWatchingMetadataRequests;
    void finishContinueWatchingLoading();
    
    // Search state
    int m_pendingSearchRequests;
    
    // Item details loading
    QString m_pendingDetailsContentId;
    QString m_pendingDetailsType;
    QString m_pendingDetailsAddonId;
};

#endif // LIBRARY_SERVICE_H

