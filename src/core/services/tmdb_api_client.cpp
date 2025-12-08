#include "tmdb_api_client.h"
#include "configuration.h"
#include "cache_service.h"
#include "logging_service.h"
#include "error_service.h"
#include <QJsonParseError>
#include <QEventLoop>

TmdbApiClient::TmdbApiClient(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_queueTimer(new QTimer(this))
    , m_requestCount(0)
    , m_windowStartTime(QDateTime::currentMSecsSinceEpoch())
    , m_isProcessingQueue(false)
    , m_requestTimeoutMs(30000) // 30 seconds default
{
    m_queueTimer->setSingleShot(true);
    connect(m_queueTimer, &QTimer::timeout, this, &TmdbApiClient::processRequestQueue);
}

TmdbApiClient::~TmdbApiClient()
{
    m_queueTimer->stop();
}

bool TmdbApiClient::isValid() const
{
    Configuration& config = Configuration::instance();
    return !config.tmdbApiKey().isEmpty() && !config.tmdbBaseUrl().isEmpty();
}

QStringList TmdbApiClient::validationErrors() const
{
    QStringList errors;
    Configuration& config = Configuration::instance();
    
    if (config.tmdbApiKey().isEmpty()) {
        errors << "TMDB API key is not configured";
    }
    if (config.tmdbBaseUrl().isEmpty()) {
        errors << "TMDB base URL is not configured";
    }
    
    return errors;
}

QUrl TmdbApiClient::buildUrl(const QString& path, const QUrlQuery& query)
{
    Configuration& config = Configuration::instance();
    QUrl url(config.tmdbBaseUrl() + path);
    
    QUrlQuery finalQuery(query);
    finalQuery.addQueryItem("api_key", config.tmdbApiKey());
    url.setQuery(finalQuery);
    
    return url;
}

bool TmdbApiClient::canMakeRequest()
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 elapsed = (now - m_windowStartTime) / 1000; // Convert to seconds
    
    // Reset window if it's been more than WINDOW_SECONDS
    if (elapsed >= WINDOW_SECONDS) {
        m_requestCount = 0;
        m_windowStartTime = now;
        return true;
    }
    
    // Check if we're under the limit
    return m_requestCount < MAX_REQUESTS_PER_WINDOW;
}

void TmdbApiClient::recordRequest()
{
    m_requestCount++;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 elapsed = (now - m_windowStartTime) / 1000;
    
    // Reset window if needed
    if (elapsed >= WINDOW_SECONDS) {
        m_requestCount = 1;
        m_windowStartTime = now;
    }
}

QString TmdbApiClient::getCacheKey(const QString& path, const QUrlQuery& query) const
{
    return CacheService::generateKeyFromQuery("tmdb", path, query);
}

// Caching now handled by CacheService - methods removed

int TmdbApiClient::getTtlForEndpoint(const QString& path) const
{
    // Different TTLs for different endpoint types
    if (path.contains("/movie/") || path.contains("/tv/")) {
        return 3600; // 1 hour for metadata
    } else if (path.contains("/search/")) {
        return 60; // 1 minute for search
    } else if (path.contains("/similar")) {
        return 1800; // 30 minutes for similar items
    }
    return 300; // 5 minutes default
}

QNetworkReply* TmdbApiClient::executeRequest(const QString& path, const QUrlQuery& query, 
                                               const QString& method, const QJsonObject& data)
{
    if (!isValid()) {
        TmdbErrorInfo errorInfo;
        errorInfo.type = TmdbError::Unauthorized;
        errorInfo.message = "TMDB API key not configured";
        errorInfo.endpoint = path;
        emit error(errorInfo);
        return nullptr;
    }
    
    QUrl url = buildUrl(path, query);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    request.setTransferTimeout(m_requestTimeoutMs);
    
    QNetworkReply* reply = nullptr;
    if (method == "GET") {
        reply = m_networkManager->get(request);
    } else if (method == "POST") {
        reply = m_networkManager->post(request, QJsonDocument(data).toJson());
    }
    
    if (reply) {
        reply->setProperty("path", path);
        reply->setProperty("method", method);
        // Store query as string for reconstruction
        if (!query.isEmpty()) {
            reply->setProperty("query", query.toString());
        }
        connect(reply, &QNetworkReply::finished, this, &TmdbApiClient::onReplyFinished);
        connect(reply, &QNetworkReply::errorOccurred, this, &TmdbApiClient::onReplyError);
    }
    
    return reply;
}

QNetworkReply* TmdbApiClient::get(const QString& path, const QUrlQuery& query)
{
    QString cacheKey = getCacheKey(path, query);
    QJsonObject cached = CacheService::getJsonCache(cacheKey);
    
    if (!cached.isEmpty()) {
        // Cache hit - emit cached response asynchronously and skip network request
        LoggingService::logDebug("TmdbApiClient", QString("Cache hit for: %1").arg(cacheKey));
        QTimer::singleShot(0, this, [this, path, query, cached]() {
            emit cachedResponseReady(path, query, cached);
        });
        return nullptr; // No network request needed
    }
    
    if (!canMakeRequest()) {
        // Queue the request
        QueuedRequest queued;
        queued.path = path;
        queued.query = query;
        queued.method = "GET";
        queued.receiver = nullptr;
        queued.slot = nullptr;
        m_requestQueue.enqueue(queued);
        
        if (!m_queueTimer->isActive()) {
            qint64 now = QDateTime::currentMSecsSinceEpoch();
            qint64 elapsed = (now - m_windowStartTime) / 1000;
            int waitTime = WINDOW_SECONDS - elapsed;
            if (waitTime > 0) {
                m_queueTimer->start(waitTime * 1000);
            } else {
                processRequestQueue();
            }
        }
        return nullptr;
    }
    
    recordRequest();
    return executeRequest(path, query, "GET");
}

QNetworkReply* TmdbApiClient::post(const QString& path, const QJsonObject& data)
{
    if (!canMakeRequest()) {
        QueuedRequest queued;
        queued.path = path;
        queued.query = QUrlQuery();
        queued.method = "POST";
        queued.data = data;
        queued.receiver = nullptr;
        queued.slot = nullptr;
        m_requestQueue.enqueue(queued);
        
        if (!m_queueTimer->isActive()) {
            qint64 now = QDateTime::currentMSecsSinceEpoch();
            qint64 elapsed = (now - m_windowStartTime) / 1000;
            int waitTime = WINDOW_SECONDS - elapsed;
            if (waitTime > 0) {
                m_queueTimer->start(waitTime * 1000);
            } else {
                processRequestQueue();
            }
        }
        return nullptr;
    }
    
    recordRequest();
    return executeRequest(path, QUrlQuery(), "POST", data);
}

void TmdbApiClient::processRequestQueue()
{
    if (m_isProcessingQueue || m_requestQueue.isEmpty()) {
        return;
    }
    
    m_isProcessingQueue = true;
    
    while (!m_requestQueue.isEmpty() && canMakeRequest()) {
        QueuedRequest request = m_requestQueue.dequeue();
        recordRequest();
        
        if (request.method == "GET") {
            executeRequest(request.path, request.query, "GET");
        } else if (request.method == "POST") {
            executeRequest(request.path, request.query, "POST", request.data);
        }
        
        // Small delay between requests
        if (!m_requestQueue.isEmpty()) {
            QEventLoop loop;
            QTimer::singleShot(MIN_API_INTERVAL_MS, &loop, &QEventLoop::quit);
            loop.exec();
        }
    }
    
    m_isProcessingQueue = false;
    
    // If queue still has items, schedule next processing
    if (!m_requestQueue.isEmpty()) {
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        qint64 elapsed = (now - m_windowStartTime) / 1000;
        int waitTime = WINDOW_SECONDS - elapsed;
        if (waitTime > 0) {
            m_queueTimer->start(waitTime * 1000);
        }
    }
}

void TmdbApiClient::onReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    // Cache successful responses
    if (reply->error() == QNetworkReply::NoError) {
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode == 200) {
            QString path = reply->property("path").toString();
            QUrlQuery query;
            // Reconstruct query from stored property or URL
            QString queryString = reply->property("query").toString();
            if (!queryString.isEmpty()) {
                query = QUrlQuery(queryString);
            } else {
                // Fallback to URL query
                QUrl url = reply->url();
                if (url.hasQuery()) {
                    query = QUrlQuery(url.query());
                }
            }
            
            // Read and parse response for caching (peek doesn't consume data)
            QByteArray responseData = reply->peek(reply->bytesAvailable());
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
            
            if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
                QString cacheKey = getCacheKey(path, query);
                int ttlSeconds = getTtlForEndpoint(path);
                CacheService::setJsonCache(cacheKey, doc.object(), ttlSeconds);
            }
        }
    }
    
    // The reply will be deleted by the service handlers after they process it
}

void TmdbApiClient::onReplyError()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    QString path = reply->property("path").toString();
    handleError(reply, path);
}

TmdbErrorInfo TmdbApiClient::createErrorInfo(QNetworkReply* reply, const QString& context) const
{
    TmdbErrorInfo errorInfo;
    errorInfo.endpoint = context;
    
    if (!reply) {
        errorInfo.type = TmdbError::NetworkError;
        errorInfo.message = "Network reply is null";
        return errorInfo;
    }
    
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    errorInfo.httpStatusCode = statusCode;
    QString errorString = reply->errorString();
    
    if (reply->error() == QNetworkReply::OperationCanceledError) {
        errorInfo.type = TmdbError::NetworkError;
        errorInfo.message = "Request canceled";
    } else if (reply->error() == QNetworkReply::TimeoutError) {
        errorInfo.type = TmdbError::Timeout;
        errorInfo.message = "Request timeout";
    } else if (statusCode == 429) {
        errorInfo.type = TmdbError::RateLimited;
        errorInfo.message = "Rate limited";
    } else if (statusCode == 401 || statusCode == 403) {
        errorInfo.type = TmdbError::Unauthorized;
        errorInfo.message = "Unauthorized - check API key";
    } else if (statusCode == 404) {
        errorInfo.type = TmdbError::NotFound;
        errorInfo.message = "Resource not found";
    } else if (statusCode >= 500) {
        errorInfo.type = TmdbError::ServerError;
        errorInfo.message = QString("Server error: %1").arg(statusCode);
    } else {
        errorInfo.type = TmdbError::NetworkError;
        errorInfo.message = errorString.isEmpty() ? "Network error" : errorString;
    }
    
    return errorInfo;
}

void TmdbApiClient::handleError(QNetworkReply* reply, const QString& context)
{
    TmdbErrorInfo errorInfo = createErrorInfo(reply, context);
    
    if (errorInfo.type == TmdbError::RateLimited) {
        qWarning() << "[TmdbApiClient] Rate limited for" << context;
    } else {
        qWarning() << "[TmdbApiClient] Error for" << context << ":" << errorInfo.message;
    }
    
    emit error(errorInfo);
    
    if (reply) {
        reply->deleteLater();
    }
}

void TmdbApiClient::clearCache()
{
    // Clear all tmdb cache entries
    CacheService::instance().clear();
    LoggingService::logInfo("TmdbApiClient", "Cache cleared");
}

void TmdbApiClient::clearCacheForEndpoint(const QString& endpoint)
{
    // Clear cache entries for specific endpoint
    // Note: CacheService doesn't support prefix-based clearing yet
    // For now, we'll clear all cache (can be optimized later)
    CacheService::instance().clear();
    LoggingService::logInfo("TmdbApiClient", QString("Cleared cache for endpoint: %1").arg(endpoint));
}

int TmdbApiClient::getCacheSize() const
{
    return CacheService::instance().size();
}

void TmdbApiClient::setRequestTimeout(int timeoutMs)
{
    m_requestTimeoutMs = timeoutMs;
}

