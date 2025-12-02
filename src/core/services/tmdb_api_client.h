#ifndef TMDB_API_CLIENT_H
#define TMDB_API_CLIENT_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QUrlQuery>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QMap>
#include <QQueue>
#include <QTimer>

struct CachedMetadata {
    QJsonObject data;
    QDateTime timestamp;
    static constexpr int ttlSeconds = 300; // 5 minutes default
    
    bool isExpired() const {
        return QDateTime::currentDateTime().secsTo(timestamp) < -ttlSeconds;
    }
};

enum class TmdbError {
    None,
    NetworkError,
    RateLimited,
    NotFound,
    Unauthorized,
    ServerError,
    ParseError,
    Timeout
};

struct TmdbErrorInfo {
    TmdbError type = TmdbError::None;
    QString message;
    int httpStatusCode = 0;
    QString endpoint;
    
    bool isValid() const { return type != TmdbError::None; }
};

class TmdbApiClient : public QObject
{
    Q_OBJECT

public:
    explicit TmdbApiClient(QObject* parent = nullptr);
    ~TmdbApiClient();
    
    // HTTP methods
    QNetworkReply* get(const QString& path, const QUrlQuery& query = QUrlQuery());
    QNetworkReply* post(const QString& path, const QJsonObject& data = QJsonObject());
    
    // URL building
    QUrl buildUrl(const QString& path, const QUrlQuery& query = QUrlQuery());
    
    // Cache management
    void clearCache();
    void clearCacheForEndpoint(const QString& endpoint);
    int getCacheSize() const;
    
    // Configuration validation
    bool isValid() const;
    QStringList validationErrors() const;
    
    // Request timeout (in milliseconds)
    void setRequestTimeout(int timeoutMs);
    int requestTimeout() const { return m_requestTimeoutMs; }

signals:
    void error(const TmdbErrorInfo& errorInfo);

private slots:
    void onReplyFinished();
    void onReplyError();
    void processRequestQueue();

private:
    struct QueuedRequest {
        QString path;
        QUrlQuery query;
        QString method;
        QJsonObject data;
        QObject* receiver;
        const char* slot;
    };
    
    static constexpr int MAX_REQUESTS_PER_WINDOW = 40;
    static constexpr int WINDOW_SECONDS = 10;
    static constexpr int MIN_API_INTERVAL_MS = (WINDOW_SECONDS * 1000) / MAX_REQUESTS_PER_WINDOW; // 250ms
    
    QNetworkAccessManager* m_networkManager;
    QMap<QString, CachedMetadata> m_cache;
    QQueue<QueuedRequest> m_requestQueue;
    QTimer* m_queueTimer;
    
    int m_requestCount;
    qint64 m_windowStartTime;
    bool m_isProcessingQueue;
    int m_requestTimeoutMs;
    
    // Rate limiting
    bool canMakeRequest();
    void recordRequest();
    
    // Caching
    QString getCacheKey(const QString& path, const QUrlQuery& query) const;
    QJsonObject getCachedResponse(const QString& cacheKey) const;
    void cacheResponse(const QString& cacheKey, const QJsonObject& data);
    int getTtlForEndpoint(const QString& path) const;
    
    // Error handling
    TmdbErrorInfo createErrorInfo(QNetworkReply* reply, const QString& context) const;
    void handleError(QNetworkReply* reply, const QString& context);
    
    // Request processing
    QNetworkReply* executeRequest(const QString& path, const QUrlQuery& query, 
                                  const QString& method = "GET", const QJsonObject& data = QJsonObject());
};

#endif // TMDB_API_CLIENT_H


