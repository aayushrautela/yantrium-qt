#include "trakt_cache_helper.h"
#include "cache_service.h"

QString TraktCacheHelper::getCacheKey(const QString& endpoint, const QUrlQuery& query)
{
    return CacheService::generateKeyFromQuery("trakt", endpoint, query);
}

int TraktCacheHelper::getTtlForEndpoint(const QString& endpoint)
{
    // Different TTLs for different endpoint types
    if (endpoint.contains("/users/me")) {
        return 3600; // 1 hour for user profile
    } else if (endpoint.contains("/sync/watched") || endpoint.contains("/sync/collection") || endpoint.contains("/sync/watchlist")) {
        return 1800; // 30 minutes for watched/collection/watchlist
    } else if (endpoint.contains("/sync/playback")) {
        return 300; // 5 minutes for playback progress
    } else if (endpoint.contains("/sync/ratings")) {
        return 3600; // 1 hour for ratings
    }
    return 300; // 5 minutes default
}
