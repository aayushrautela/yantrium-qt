#ifndef FRONTEND_DATA_MAPPER_H
#define FRONTEND_DATA_MAPPER_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantMap>
#include <QVariantList>

/// Centralized backend-to-frontend data transformation utility
/// Ensures consistent field names and data structures for QML
class FrontendDataMapper
{
public:
    // Date formatting
    static QString formatDateDDMMYYYY(const QString& dateString);
    
    // TMDB to Catalog Item (QJsonObject) - for catalog listings
    static QJsonObject mapTmdbToCatalogItem(const QJsonObject& tmdbData, const QString& contentId, const QString& type);
    
    // TMDB to Detail VariantMap - for detail pages
    static QVariantMap mapTmdbToDetailVariantMap(const QJsonObject& tmdbData, const QString& contentId, const QString& type);
    
    // Stremio/Addon metadata to Detail VariantMap - for detail pages
    static QVariantMap mapAddonMetaToDetailVariantMap(const QJsonObject& addonMeta, const QString& contentId, const QString& type);
    
    // Catalog Item to VariantMap - for QML display
    static QVariantMap mapCatalogItemToVariantMap(const QJsonObject& item, const QString& baseUrl = QString());
    
    // Continue Watching Item - combines Trakt + TMDB data
    static QVariantMap mapContinueWatchingItem(const QVariantMap& traktItem, const QJsonObject& tmdbData = QJsonObject());
    
    // Search Results to VariantList - for search results
    static QVariantList mapSearchResultsToVariantList(const QJsonArray& results, [[maybe_unused]] const QString& mediaType);
    
    // Similar Items to VariantList - for similar content
    static QVariantList mapSimilarItemsToVariantList(const QJsonArray& results, const QString& type);
    
    // Merge OMDB ratings into detail map
    static QVariantMap mergeOmdbRatings(QVariantMap& detailMap, const QJsonObject& omdbData);

    // Enrich catalog item with TMDB data (for hero items)
    static QVariantMap enrichItemWithTmdbData(const QVariantMap& item, const QJsonObject& tmdbData, const QString& type);

    // Helper functions for TMDB data processing
    static QString determineBadgeText(const QJsonObject& tmdbData, const QString& type);
    static QString formatRuntime(int minutes);
};

#endif // FRONTEND_DATA_MAPPER_H



