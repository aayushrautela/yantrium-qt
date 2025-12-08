#include "cache_service.h"
#include "logging_service.h"
#include "core/di/service_registry.h"
#include <QUrlQuery>

CacheService& CacheService::instance()
{
    static CacheService* s_instance = nullptr;
    if (!s_instance) {
        s_instance = new CacheService();
    }
    return *s_instance;
}

CacheService::CacheService(QObject* parent)
    : QObject(parent)
{
    LoggingService::logInfo("CacheService", "Initialized");
}

void CacheService::set(const QString& key, const QVariant& data, int ttlSeconds)
{
    if (key.isEmpty()) {
        LoggingService::logWarning("CacheService", "set called with empty key");
        return;
    }

    CacheEntry entry;
    entry.data = data;
    entry.timestamp = QDateTime::currentDateTime();
    entry.ttlSeconds = ttlSeconds;
    entry.isJson = false;

    m_cache[key] = entry;
    LoggingService::logDebug("CacheService", QString("Cached entry: %1 (TTL: %2s)").arg(key).arg(ttlSeconds));
}

void CacheService::setJson(const QString& key, const QJsonObject& data, int ttlSeconds)
{
    if (key.isEmpty()) {
        LoggingService::logWarning("CacheService", "setJson called with empty key");
        return;
    }

    CacheEntry entry;
    entry.data = QVariant::fromValue(data);
    entry.timestamp = QDateTime::currentDateTime();
    entry.ttlSeconds = ttlSeconds;
    entry.isJson = true;

    m_cache[key] = entry;
    LoggingService::logDebug("CacheService", QString("Cached JSON entry: %1 (TTL: %2s)").arg(key).arg(ttlSeconds));
}

QVariant CacheService::get(const QString& key) const
{
    cleanupExpired();

    if (!m_cache.contains(key)) {
        return QVariant();
    }

    const CacheEntry& entry = m_cache[key];
    if (entry.isExpired()) {
        m_cache.remove(key);
        return QVariant();
    }

    return entry.data;
}

QJsonObject CacheService::getJson(const QString& key) const
{
    QVariant data = get(key);
    if (!data.isValid()) {
        return QJsonObject();
    }

    if (data.canConvert<QJsonObject>()) {
        return data.toJsonObject();
    }

    return QJsonObject();
}

bool CacheService::contains(const QString& key) const
{
    cleanupExpired();
    return m_cache.contains(key) && !m_cache[key].isExpired();
}

void CacheService::remove(const QString& key)
{
    if (m_cache.remove(key) > 0) {
        emit cacheEntryRemoved(key);
        LoggingService::logDebug("CacheService", QString("Removed cache entry: %1").arg(key));
    }
}

void CacheService::clear()
{
    int size = m_cache.size();
    m_cache.clear();
    emit cacheCleared();
    LoggingService::logInfo("CacheService", QString("Cleared %1 cache entries").arg(size));
}

void CacheService::clearExpired()
{
    cleanupExpired();
}

int CacheService::size() const
{
    cleanupExpired();
    return m_cache.size();
}

QString CacheService::generateKey(const QString& service, const QString& endpoint, const QString& params)
{
    QString key = service + ":" + endpoint;
    if (!params.isEmpty()) {
        key += ":" + params;
    }
    return key;
}

QString CacheService::generateKeyFromQuery(const QString& service, const QString& endpoint, const QUrlQuery& query)
{
    QString params;
    if (!query.isEmpty()) {
        QStringList items;
        for (const auto& item : query.queryItems()) {
            items << QString("%1=%2").arg(item.first, item.second);
        }
        items.sort();
        params = items.join("&");
    }
    return generateKey(service, endpoint, params);
}

void CacheService::setCache(const QString& key, const QVariant& data, int ttlSeconds)
{
    auto service = ServiceRegistry::instance().resolve<CacheService>();
    if (service) {
        service->set(key, data, ttlSeconds);
    }
}

void CacheService::setJsonCache(const QString& key, const QJsonObject& data, int ttlSeconds)
{
    auto service = ServiceRegistry::instance().resolve<CacheService>();
    if (service) {
        service->setJson(key, data, ttlSeconds);
    }
}

QVariant CacheService::getCache(const QString& key)
{
    auto service = ServiceRegistry::instance().resolve<CacheService>();
    return service ? service->get(key) : QVariant();
}

QJsonObject CacheService::getJsonCache(const QString& key)
{
    auto service = ServiceRegistry::instance().resolve<CacheService>();
    return service ? service->getJson(key) : QJsonObject();
}

bool CacheService::hasCache(const QString& key)
{
    auto service = ServiceRegistry::instance().resolve<CacheService>();
    return service ? service->contains(key) : false;
}

void CacheService::removeCache(const QString& key)
{
    auto service = ServiceRegistry::instance().resolve<CacheService>();
    if (service) {
        service->remove(key);
    }
}

void CacheService::cleanupExpired() const
{
    QList<QString> expiredKeys;
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        if (it.value().isExpired()) {
            expiredKeys.append(it.key());
        }
    }

    for (const QString& key : expiredKeys) {
        m_cache.remove(key);
    }

    if (!expiredKeys.isEmpty()) {
        LoggingService::logDebug("CacheService", QString("Cleaned up %1 expired cache entries").arg(expiredKeys.size()));
    }
}

