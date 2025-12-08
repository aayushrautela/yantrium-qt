#ifndef TRAKT_CACHE_HELPER_H
#define TRAKT_CACHE_HELPER_H

#include <QString>
#include <QUrlQuery>

/// Internal helper for Trakt API caching logic
namespace TraktCacheHelper
{
    /// Generate cache key for an endpoint
    QString getCacheKey(const QString& endpoint, const QUrlQuery& query = QUrlQuery());
    
    /// Get TTL (time to live) in seconds for an endpoint
    int getTtlForEndpoint(const QString& endpoint);
}

#endif // TRAKT_CACHE_HELPER_H
