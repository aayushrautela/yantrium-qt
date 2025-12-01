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
#include "core/database/catalog_preferences_dao.h"

class LibraryService : public QObject
{
    Q_OBJECT

public:
    explicit LibraryService(QObject* parent = nullptr);
    ~LibraryService();
    
    Q_INVOKABLE void loadCatalogs();
    Q_INVOKABLE void loadCatalog(const QString& addonId, const QString& type, const QString& id = QString());
    Q_INVOKABLE void searchCatalogs(const QString& query);
    Q_INVOKABLE QVariantList getCatalogSections();
    Q_INVOKABLE QVariantList getContinueWatching();
    Q_INVOKABLE void loadCatalogsRaw(); // Load catalogs without processing (for raw export)
    Q_INVOKABLE void loadHeroItems(); // Load items from hero catalogs
    
private:
    void loadCatalogsRaw(bool rawMode); // Internal method with raw mode flag
    
signals:
    void catalogsLoaded(const QVariantList& sections);
    void continueWatchingLoaded(const QVariantList& items);
    void searchResultsLoaded(const QVariantList& results);
    void rawCatalogsLoaded(const QVariantList& rawData); // Raw unprocessed catalog data
    void heroItemsLoaded(const QVariantList& items); // Hero items loaded
    void error(const QString& message);

private slots:
    void onCatalogFetched(const QString& type, const QJsonArray& metas);
    void onClientError(const QString& errorMessage);
    void onPlaybackProgressFetched(const QVariantList& progress);
    void onHeroCatalogFetched(const QString& type, const QJsonArray& metas);
    void onHeroClientError(const QString& errorMessage);

private:
    struct CatalogSection {
        QString name;
        QString type;
        QString addonId;
        QVariantList items;
    };
    
    void processCatalogData(const QString& addonId, const QString& catalogName, const QString& type, const QJsonArray& metas);
    QVariantMap catalogItemToVariantMap(const QJsonObject& item, const QString& baseUrl = QString());
    QVariantMap traktPlaybackItemToVariantMap(const QVariantMap& traktItem);
    void finishLoadingCatalogs();
    
    AddonRepository* m_addonRepository;
    TraktCoreService* m_traktService;
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
};

#endif // LIBRARY_SERVICE_H

