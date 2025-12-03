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
#include "features/addons/logic/addon_repository.h"
#include "features/addons/logic/addon_client.h"
#include "features/addons/models/addon_config.h"
#include "features/addons/models/addon_manifest.h"
#include "core/services/trakt_core_service.h"
#include "core/services/tmdb_data_service.h"
#include "core/services/tmdb_search_service.h"
#include "core/services/media_metadata_service.h"
#include "core/services/tmdb_data_mapper.h"
#include "core/services/frontend_data_mapper.h"
#include "core/services/omdb_service.h"
#include "core/database/catalog_preferences_dao.h"

class DatabaseManager;
class LocalLibraryService;

class LibraryService : public QObject
{
    Q_OBJECT

public:
    explicit LibraryService(QObject* parent = nullptr);
    ~LibraryService();
    
    Q_INVOKABLE void loadCatalogs();
    Q_INVOKABLE void loadCatalog(const QString& addonId, const QString& type, const QString& id = QString());
    Q_INVOKABLE void searchCatalogs(const QString& query);
    Q_INVOKABLE void searchTmdb(const QString& query);  // Search TMDB for movies and TV
    Q_INVOKABLE QVariantList getCatalogSections();
    Q_INVOKABLE QVariantList getContinueWatching();
    Q_INVOKABLE void loadCatalogsRaw(); // Load catalogs without processing (for raw export)
    Q_INVOKABLE void loadHeroItems(); // Load items from hero catalogs
    Q_INVOKABLE void loadItemDetails(const QString& contentId, const QString& type, const QString& addonId = QString());
    Q_INVOKABLE void loadSimilarItems(int tmdbId, const QString& type);
    Q_INVOKABLE void getSmartPlayState(const QVariantMap& itemData);
    Q_INVOKABLE void loadSeasonEpisodes(int tmdbId, int seasonNumber);

    // Cache management
    Q_INVOKABLE void clearMetadataCache();
    Q_INVOKABLE int getMetadataCacheSize() const;
    
private:
    void loadCatalogsRaw(bool rawMode); // Internal method with raw mode flag
    
signals:
    void catalogsLoaded(const QVariantList& sections);
    void continueWatchingLoaded(const QVariantList& items);
    void searchResultsLoaded(const QVariantList& results);
    void tmdbSearchResultsLoaded(const QVariantList& results);  // Combined movies + TV results
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
    void onTmdbMovieMetadataFetched(int tmdbId, const QJsonObject& data);
    void onTmdbTvMetadataFetched(int tmdbId, const QJsonObject& data);
    void onTmdbIdFound(const QString& imdbId, int tmdbId);
    void onTmdbError(const QString& message);
    void onHeroCatalogFetched(const QString& type, const QJsonArray& metas);
    void onHeroClientError(const QString& errorMessage);
    void onMediaMetadataLoaded(const QVariantMap& details);
    void onMediaMetadataError(const QString& message);
    void onWatchProgressLoaded(const QVariantMap& progress);
    void onSimilarMoviesFetched(int tmdbId, const QJsonArray& results);
    void onSimilarTvFetched(int tmdbId, const QJsonArray& results);
    void onSearchMoviesFound(const QVariantList& results);
    void onSearchTvFound(const QVariantList& results);
    void onSearchError(const QString& message);
    void onTvSeasonDetailsFetched(int tmdbId, int seasonNumber, const QJsonObject& data);

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
    
    AddonRepository* m_addonRepository;
    TraktCoreService* m_traktService;
    TmdbDataService* m_tmdbService;
    TmdbSearchService* m_tmdbSearchService;
    MediaMetadataService* m_mediaMetadataService;
    OmdbService* m_omdbService;
    LocalLibraryService* m_localLibraryService;
    CatalogPreferencesDao* m_catalogPreferencesDao;
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
    
    // Continue watching TMDB loading
    QMap<QString, QVariantMap> m_pendingContinueWatchingItems; // IMDB ID -> Trakt item data
    QMap<QString, QVariantMap> m_pendingSmartPlayItems; // Content ID -> item data for smart play
    QMap<int, QString> m_tmdbIdToImdbId; // TMDB ID -> IMDB ID
    int m_pendingTmdbRequests;
    void finishContinueWatchingLoading();
    
    // Search state
    QString m_pendingSearchQuery;
    QVariantList m_pendingSearchMovies;
    QVariantList m_pendingSearchTv;
    bool m_searchMoviesReceived;
    bool m_searchTvReceived;
    void finishSearch();
    
    // Item details loading
    QString m_pendingDetailsContentId;
    QString m_pendingDetailsType;
    QString m_pendingDetailsAddonId;
    QMap<int, QString> m_tmdbIdToImdbIdForDetails; // TMDB ID -> IMDB ID for details
};

#endif // LIBRARY_SERVICE_H

