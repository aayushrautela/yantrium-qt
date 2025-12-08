#ifndef CACHE_SERVICE_H
#define CACHE_SERVICE_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QMap>
#include <QUrlQuery>
#include <QtQmlIntegration/qqmlintegration.h>

/**
 * @brief Unified cache service for all data types
 * 
 * Provides consistent caching interface for QJsonObject, QVariantMap, and QVariantList
 * with TTL-based expiration and cache management.
 */
class CacheService : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit CacheService(QObject* parent = nullptr);

    /**
     * @brief Get singleton instance (for C++ usage)
     */
    static CacheService& instance();

    /**
     * @brief Store QVariant data in cache
     * @param key Cache key
     * @param data Data to cache
     * @param ttlSeconds Time to live in seconds (default: 300 = 5 minutes)
     */
    Q_INVOKABLE void set(const QString& key, const QVariant& data, int ttlSeconds = 300);

    /**
     * @brief Store QJsonObject in cache
     * @param key Cache key
     * @param data JSON data to cache
     * @param ttlSeconds Time to live in seconds
     */
    Q_INVOKABLE void setJson(const QString& key, const QJsonObject& data, int ttlSeconds = 300);

    /**
     * @brief Retrieve QVariant data from cache
     * @param key Cache key
     * @return Cached data or invalid QVariant if not found/expired
     */
    Q_INVOKABLE QVariant get(const QString& key) const;

    /**
     * @brief Retrieve QJsonObject from cache
     * @param key Cache key
     * @return Cached JSON object or empty object if not found/expired
     */
    Q_INVOKABLE QJsonObject getJson(const QString& key) const;

    /**
     * @brief Check if key exists and is not expired
     */
    Q_INVOKABLE bool contains(const QString& key) const;

    /**
     * @brief Remove specific key from cache
     */
    Q_INVOKABLE void remove(const QString& key);

    /**
     * @brief Clear all cache entries
     */
    Q_INVOKABLE void clear();

    /**
     * @brief Clear all expired cache entries
     */
    Q_INVOKABLE void clearExpired();

    /**
     * @brief Get current cache size (number of entries)
     */
    Q_INVOKABLE int size() const;

    /**
     * @brief Generate cache key from service, endpoint, and parameters
     * Format: service:endpoint:params
     */
    static QString generateKey(const QString& service, const QString& endpoint, const QString& params = "");

    /**
     * @brief Generate cache key from service, endpoint, and query parameters
     */
    static QString generateKeyFromQuery(const QString& service, const QString& endpoint, const QUrlQuery& query);

    /**
     * @brief C++ convenience methods (public for service access)
     */
    static void setCache(const QString& key, const QVariant& data, int ttlSeconds = 300);
    static void setJsonCache(const QString& key, const QJsonObject& data, int ttlSeconds = 300);
    static QVariant getCache(const QString& key);
    static QJsonObject getJsonCache(const QString& key);
    static bool hasCache(const QString& key);
    static void removeCache(const QString& key);

signals:
    /**
     * @brief Emitted when cache is cleared
     */
    void cacheCleared();

    /**
     * @brief Emitted when cache entry is removed
     */
    void cacheEntryRemoved(const QString& key);

private:
    struct CacheEntry {
        QVariant data;
        QDateTime timestamp;
        int ttlSeconds;
        bool isJson;

        CacheEntry() : ttlSeconds(300), isJson(false) {}

        bool isExpired() const {
            return QDateTime::currentDateTime().secsTo(timestamp) < -ttlSeconds;
        }
    };

    mutable QMap<QString, CacheEntry> m_cache;
    static CacheService* s_instance;

    void cleanupExpired() const;
};

#endif // CACHE_SERVICE_H

