#ifndef ILIBRARY_SERVICE_H
#define ILIBRARY_SERVICE_H

#include <QString>
#include <QVariantList>
#include <QVariantMap>

/**
 * @brief Interface for library service operations
 * 
 * Pure abstract interface - no QObject inheritance to avoid diamond inheritance.
 * Signals are defined in concrete implementations.
 */
class ILibraryService
{
public:
    virtual ~ILibraryService() = default;
    
    virtual void loadCatalogs() = 0;
    virtual void loadCatalog(const QString& addonId, const QString& type, const QString& id = QString()) = 0;
    virtual void searchCatalogs(const QString& query) = 0;
    virtual void searchTmdb(const QString& query) = 0;
    [[nodiscard]] virtual QVariantList getCatalogSections() = 0;
    [[nodiscard]] virtual QVariantList getContinueWatching() = 0;
    virtual void loadCatalogsRaw() = 0;
    virtual void loadHeroItems() = 0;
    virtual void loadItemDetails(const QString& contentId, const QString& type, const QString& addonId = QString()) = 0;
    virtual void loadSimilarItems(int tmdbId, const QString& type) = 0;
    virtual void getSmartPlayState(const QVariantMap& itemData) = 0;
    virtual void loadSeasonEpisodes(int tmdbId, int seasonNumber) = 0;
    
    // Cache management
    virtual void clearMetadataCache() = 0;
    [[nodiscard]] virtual int getMetadataCacheSize() const = 0;
};

#endif // ILIBRARY_SERVICE_H

