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
    
    // Catalog Item to VariantMap - for QML display
    static QVariantMap mapCatalogItemToVariantMap(const QJsonObject& item, const QString& baseUrl = QString());
    
    // Continue Watching Item - combines Trakt + TMDB data
    static QVariantMap mapContinueWatchingItem(const QVariantMap& traktItem, const QJsonObject& tmdbData = QJsonObject());
    
    // Search Results to VariantList - for search results
    static QVariantList mapSearchResultsToVariantList(const QJsonArray& results, const QString& mediaType);
    
    // Similar Items to VariantList - for similar content
    static QVariantList mapSimilarItemsToVariantList(const QJsonArray& results, const QString& type);
    
    // Merge OMDB ratings into detail map
    static QVariantMap mergeOmdbRatings(QVariantMap& detailMap, const QJsonObject& omdbData);
};

#endif // FRONTEND_DATA_MAPPER_H


