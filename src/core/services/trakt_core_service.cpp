#include "trakt_core_service.h"
#include "configuration.h"
#include "../database/database_manager.h"
#include "../database/sync_tracking_dao.h"
#include "../database/watch_history_dao.h"
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include <QTimer>
#include <QElapsedTimer>
#include <QEventLoop>
#include <cmath>
#include <QVariantMap>
#include <QUrl>

TraktCoreService& TraktCoreService::instance()
{
    static TraktCoreService instance;
    return instance;
}

TraktCoreService::TraktCoreService(QObject* parent)
    : QObject(parent)
    , m_networkManager(std::make_unique<QNetworkAccessManager>(this))
    , m_authDao(nullptr)
    , m_tokenExpiry(0)
    , m_isInitialized(false)
    , m_lastApiCall(0)
    , m_queueTimer(std::make_unique<QTimer>(this))
    , m_isProcessingQueue(false)
    , m_completionThreshold(81)  // More than 80% (>80%) is considered watched
    , m_cleanupTimer(std::make_unique<QTimer>(this))
    , m_syncDao(nullptr)
    , m_watchHistoryDao(nullptr)
{
    m_queueTimer->setSingleShot(true);
    m_queueTimer->setInterval(MIN_API_INTERVAL_MS);
    connect(m_queueTimer.get(), &QTimer::timeout, this, &TraktCoreService::processRequestQueue);
    
    m_cleanupTimer->setInterval(60000);  // Cleanup every minute
    connect(m_cleanupTimer.get(), &QTimer::timeout, this, &TraktCoreService::cleanupOldData);
    m_cleanupTimer->start();
}

TraktCoreService::~TraktCoreService() = default;

void TraktCoreService::initializeDatabase()
{
    if (m_authDao) {
        qDebug() << "[TraktCoreService] Database already initialized";
        return;
    }

    // Get database connection by name (new pattern)
    m_database = QSqlDatabase::database(DatabaseManager::CONNECTION_NAME);

    // Create DAO objects (new pattern - no database parameter)
    m_authDao = std::make_unique<TraktAuthDao>();
    m_syncDao = std::make_unique<SyncTrackingDao>();
    m_watchHistoryDao = std::make_unique<WatchHistoryDao>();
    qDebug() << "[TraktCoreService] Database initialized";
}

void TraktCoreService::initializeAuth()
{
    if (m_isInitialized) {
        return;
    }
    if (!m_database.isValid() || !m_authDao) {
        qWarning() << "[TraktCoreService] Cannot initialize auth: database not set";
        return;
    }
    
    try {
        auto auth = m_authDao->getTraktAuth();
        if (auth) {
            m_accessToken = auth->accessToken;
            m_refreshToken = auth->refreshToken;
            m_tokenExpiry = auth->expiresAt.toMSecsSinceEpoch();
        }
        m_isInitialized = true;
        qDebug() << "[TraktCoreService] Auth initialized, authenticated:" << !m_accessToken.isEmpty();
    } catch (...) {
        qWarning() << "[TraktCoreService] Auth initialization failed";
    }
}

void TraktCoreService::reloadAuth()
{
    if (!m_database.isValid() || !m_authDao) {
        qWarning() << "[TraktCoreService] Cannot reload auth: database not set";
        return;
    }
    
    try {
        auto auth = m_authDao->getTraktAuth();
        if (auth) {
            m_accessToken = auth->accessToken;
            m_refreshToken = auth->refreshToken;
            m_tokenExpiry = auth->expiresAt.toMSecsSinceEpoch();
            qDebug() << "[TraktCoreService] Auth reloaded, authenticated:" << !m_accessToken.isEmpty();
        } else {
            m_accessToken.clear();
            m_refreshToken.clear();
            m_tokenExpiry = 0;
            qDebug() << "[TraktCoreService] Auth reloaded, no auth found";
        }
    } catch (...) {
        qWarning() << "[TraktCoreService] Auth reload failed";
    }
}

QString TraktCoreService::getAccessTokenSync()
{
    if (!m_isInitialized) {
        initializeAuth();
    }
    
    if (m_accessToken.isEmpty()) {
        return QString();
    }
    
    // Check if token is expired or about to expire (within 5 minutes)
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 fiveMinutesFromNow = now + (5 * 60 * 1000);
    
    if (m_tokenExpiry > 0 && m_tokenExpiry < fiveMinutesFromNow && !m_refreshToken.isEmpty()) {
        refreshAccessToken();
    }
    
    return m_accessToken;
}

void TraktCoreService::refreshAccessToken()
{
    if (m_refreshToken.isEmpty()) {
        qWarning() << "[TraktCoreService] No refresh token available";
        return;
    }
    
    Configuration& config = Configuration::instance();
    QUrl url(config.traktTokenUrl());
    
    QJsonObject data;
    data["refresh_token"] = m_refreshToken;
    data["client_id"] = config.traktClientId();
    data["client_secret"] = config.traktClientSecret();
    data["redirect_uri"] = config.traktRedirectUri();
    data["grant_type"] = "refresh_token";
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("trakt-api-version", config.traktApiVersion().toUtf8());
    request.setRawHeader("trakt-api-key", config.traktClientId().toUtf8());
    
    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(data).toJson());
    
    // Use a synchronous wait for token refresh (blocking but necessary)
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject response = doc.object();
        
        QString accessToken = response["access_token"].toString();
        QString newRefreshToken = response["refresh_token"].toString();
        int expiresIn = response["expires_in"].toInt();
        
        saveTokens(accessToken, newRefreshToken, expiresIn);
        qDebug() << "[TraktCoreService] Access token refreshed successfully";
    } else {
        qWarning() << "[TraktCoreService] Failed to refresh token:" << reply->errorString();
        logout();  // Clear tokens if refresh fails
    }
    
    reply->deleteLater();
}

void TraktCoreService::saveTokens(const QString& accessToken, const QString& refreshToken, int expiresIn)
{
    m_accessToken = accessToken;
    m_refreshToken = refreshToken;
    m_tokenExpiry = QDateTime::currentMSecsSinceEpoch() + (expiresIn * 1000);
    
    if (m_authDao) {
        TraktAuthRecord record;
        record.accessToken = accessToken;
        record.refreshToken = refreshToken;
        record.expiresIn = expiresIn;
        record.createdAt = QDateTime::currentDateTime();
        record.expiresAt = QDateTime::fromMSecsSinceEpoch(m_tokenExpiry);
        
        (void)m_authDao->upsertTraktAuth(record);
        qDebug() << "[TraktCoreService] Tokens saved successfully";
    } else {
        qWarning() << "[TraktCoreService] Cannot save tokens: database not initialized";
    }
}

void TraktCoreService::logout()
{
    m_accessToken.clear();
    m_refreshToken.clear();
    m_tokenExpiry = 0;
    
    if (m_authDao) {
        (void)m_authDao->deleteTraktAuth();
        qDebug() << "[TraktCoreService] User logged out successfully";
    }
}

QUrl TraktCoreService::buildUrl(const QString& endpoint)
{
    Configuration& config = Configuration::instance();
    return QUrl(config.traktBaseUrl() + endpoint);
}

QString TraktCoreService::getCacheKey(const QString& endpoint, const QUrlQuery& query) const
{
    QString key = endpoint;
    if (!query.isEmpty()) {
        QStringList items;
        for (const auto& item : query.queryItems()) {
            items << QString("%1=%2").arg(item.first, item.second);
        }
        items.sort();
        key += "?" + items.join("&");
    }
    return key;
}

QJsonObject TraktCoreService::getCachedResponse(const QString& cacheKey) const
{
    if (m_cache.contains(cacheKey)) {
        const CachedTraktData& cached = m_cache[cacheKey];
        if (!cached.isExpired()) {
            return cached.data;
        }
    }
    return QJsonObject();
}

void TraktCoreService::cacheResponse(const QString& cacheKey, const QJsonObject& data, int ttlSeconds)
{
    CachedTraktData cached;
    cached.data = data;
    cached.timestamp = QDateTime::currentDateTime();
    cached.ttlSeconds = ttlSeconds;
    m_cache[cacheKey] = cached;
    qDebug() << "[TraktCoreService] Cached response for:" << cacheKey << "TTL:" << ttlSeconds << "seconds";
}

int TraktCoreService::getTtlForEndpoint(const QString& endpoint) const
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

void TraktCoreService::apiRequest(const QString& endpoint, const QString& method,
                                  const QJsonObject& data, QObject* receiver, const char* slot)
{
    // Check cache for GET requests
    if (method == "GET") {
        QString cacheKey = getCacheKey(endpoint);
        QJsonObject cached = getCachedResponse(cacheKey);
        
        if (!cached.isEmpty()) {
            // Cache hit - emit cached response asynchronously
            qDebug() << "[TraktCoreService] Cache hit for:" << cacheKey;
            QTimer::singleShot(0, this, [this, endpoint, cached]() {
                // Parse cached response based on endpoint (same logic as onApiReplyFinished)
                QJsonArray arr;
                if (cached.contains("_array")) {
                    // Array was cached as object with _array key
                    arr = cached["_array"].toArray();
                }
                
                if (endpoint.contains("/users/me")) {
                    emit userProfileFetched(cached);
                } else if (endpoint.contains("/sync/watched/movies")) {
                    QVariantList movies;
                    for (const QJsonValue& val : arr) {
                        movies.append(val.toObject().toVariantMap());
                    }
                    emit watchedMoviesFetched(movies);
                } else if (endpoint.contains("/sync/watched/shows")) {
                    QVariantList shows;
                    for (const QJsonValue& val : arr) {
                        shows.append(val.toObject().toVariantMap());
                    }
                    emit watchedShowsFetched(shows);
                } else if (endpoint.contains("/sync/watchlist/movies")) {
                    QVariantList movies;
                    for (const QJsonValue& val : arr) {
                        movies.append(val.toObject().toVariantMap());
                    }
                    emit watchlistMoviesFetched(movies);
                } else if (endpoint.contains("/sync/watchlist/shows")) {
                    QVariantList shows;
                    for (const QJsonValue& val : arr) {
                        shows.append(val.toObject().toVariantMap());
                    }
                    emit watchlistShowsFetched(shows);
                } else if (endpoint.contains("/sync/collection/movies")) {
                    QVariantList movies;
                    for (const QJsonValue& val : arr) {
                        movies.append(val.toObject().toVariantMap());
                    }
                    emit collectionMoviesFetched(movies);
                } else if (endpoint.contains("/sync/collection/shows")) {
                    QVariantList shows;
                    for (const QJsonValue& val : arr) {
                        shows.append(val.toObject().toVariantMap());
                    }
                    emit collectionShowsFetched(shows);
                } else if (endpoint.contains("/sync/ratings")) {
                    QVariantList ratings;
                    for (const QJsonValue& val : arr) {
                        ratings.append(val.toObject().toVariantMap());
                    }
                    emit ratingsFetched(ratings);
                } else if (endpoint.contains("/sync/playback")) {
                    QVariantList progress;
                    for (const QJsonValue& val : arr) {
                        progress.append(val.toObject().toVariantMap());
                    }
                    emit playbackProgressFetched(progress);
                }
            });
            return; // Skip network request
        }
    }
    
    // Rate limiting: ensure minimum interval between API calls
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 timeSinceLastCall = now - m_lastApiCall;
    
    if (timeSinceLastCall < MIN_API_INTERVAL_MS) {
        // Queue the request
        QueuedRequest request;
        request.endpoint = endpoint;
        request.method = method;
        request.data = data;
        request.receiver = receiver;
        request.slot = slot;
        m_requestQueue.enqueue(request);
        
        if (!m_queueTimer->isActive()) {
            m_queueTimer->start(MIN_API_INTERVAL_MS - timeSinceLastCall);
        }
        return;
    }
    
    m_lastApiCall = now;
    
    // Ensure we have a valid token
    QString token = getAccessTokenSync();
    if (token.isEmpty()) {
        emit error("Not authenticated");
        return;
    }
    
    QUrl url = buildUrl(endpoint);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    request.setRawHeader("trakt-api-version", Configuration::instance().traktApiVersion().toUtf8());
    request.setRawHeader("trakt-api-key", Configuration::instance().traktClientId().toUtf8());
    
    QNetworkReply* reply = nullptr;
    if (method == "GET") {
        reply = m_networkManager->get(request);
    } else if (method == "POST") {
        reply = m_networkManager->post(request, QJsonDocument(data).toJson());
    } else if (method == "PUT") {
        reply = m_networkManager->put(request, QJsonDocument(data).toJson());
    } else if (method == "DELETE") {
        reply = m_networkManager->deleteResource(request);
    }
    
    if (reply) {
        reply->setProperty("endpoint", endpoint);
        reply->setProperty("method", method);
        if (receiver && slot) {
            // Use old-style connect for const char* slot names
            connect(reply, SIGNAL(finished()), receiver, slot);
        } else {
            connect(reply, &QNetworkReply::finished, this, &TraktCoreService::onApiReplyFinished);
        }
        connect(reply, &QNetworkReply::errorOccurred, this, [this, reply, endpoint]() {
            handleError(reply, endpoint);
        });
    }
}

void TraktCoreService::processRequestQueue()
{
    if (m_isProcessingQueue || m_requestQueue.isEmpty()) {
        return;
    }
    
    m_isProcessingQueue = true;
    
    while (!m_requestQueue.isEmpty()) {
        QueuedRequest request = m_requestQueue.dequeue();
        apiRequest(request.endpoint, request.method, request.data, request.receiver, request.slot);
        
        // Wait minimum interval before next request
        if (!m_requestQueue.isEmpty()) {
            QEventLoop loop;
            QTimer::singleShot(MIN_API_INTERVAL_MS, &loop, &QEventLoop::quit);
            loop.exec();
        }
    }
    
    m_isProcessingQueue = false;
}

void TraktCoreService::cleanupOldData()
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    int cleanupCount = 0;
    
    // Remove stop calls older than the debounce window
    auto it = m_lastStopCalls.begin();
    while (it != m_lastStopCalls.end()) {
        if (now - it.value() > STOP_DEBOUNCE_MS) {
            it = m_lastStopCalls.erase(it);
            cleanupCount++;
        } else {
            ++it;
        }
    }
    
    // Clean up old scrobbled timestamps
    it = m_scrobbledTimestamps.begin();
    while (it != m_scrobbledTimestamps.end()) {
        if (now - it.value() > SCROBBLE_EXPIRY_MS) {
            m_scrobbledItems.remove(it.key());
            it = m_scrobbledTimestamps.erase(it);
            cleanupCount++;
        } else {
            ++it;
        }
    }
    
    // Clean up old sync times
    it = m_lastSyncTimes.begin();
    while (it != m_lastSyncTimes.end()) {
        if (now - it.value() > 24 * 60 * 60 * 1000) {  // 24 hours
            it = m_lastSyncTimes.erase(it);
            cleanupCount++;
        } else {
            ++it;
        }
    }
    
    if (cleanupCount > 0) {
        qDebug() << "[TraktCoreService] Cleaned up" << cleanupCount << "old tracking entries";
    }
}

void TraktCoreService::handleError(QNetworkReply* reply, const QString& context)
{
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QString errorString = reply->errorString();
    
    if (statusCode == 429) {
        qWarning() << "[TraktCoreService] Rate limited (429) for" << context;
        emit error("Rate limited. Please try again later.");
    } else if (statusCode == 409) {
        qWarning() << "[TraktCoreService] Conflict (409) for" << context;
        // Don't emit error for conflicts, they're handled gracefully
    } else if (statusCode == 404) {
        qWarning() << "[TraktCoreService] Not found (404) for" << context;
        emit error("Content not found in Trakt database");
    } else {
        qWarning() << "[TraktCoreService] API Error" << statusCode << "for" << context << ":" << errorString;
        emit error(QString("API request failed: %1").arg(errorString));
    }
}

QString TraktCoreService::getContentKeyFromPayload(const QJsonObject& payload)
{
    if (payload.contains("movie")) {
        QJsonObject movie = payload["movie"].toObject();
        QJsonObject ids = movie["ids"].toObject();
        QString imdb = ids["imdb"].toString();
        if (!imdb.isEmpty()) {
            return "movie:" + imdb;
        }
    } else if (payload.contains("episode") && payload.contains("show")) {
        QJsonObject show = payload["show"].toObject();
        QJsonObject ids = show["ids"].toObject();
        QString imdb = ids["imdb"].toString();
        QJsonObject episode = payload["episode"].toObject();
        int season = episode["season"].toInt();
        int number = episode["number"].toInt();
        if (!imdb.isEmpty()) {
            return QString("episode:%1:S%2E%3").arg(imdb).arg(season).arg(number);
        }
    }
    return QString();
}

bool TraktCoreService::isRecentlyScrobbled(const QString& contentKey)
{
    cleanupOldData();
    return m_scrobbledItems.contains(contentKey);
}

void TraktCoreService::setCompletionThreshold(int threshold)
{
    if (threshold >= 50 && threshold <= 100) {
        m_completionThreshold = threshold;
        qDebug() << "[TraktCoreService] Updated completion threshold to:" << m_completionThreshold << "%";
    }
}

void TraktCoreService::checkAuthentication()
{
    QString token = getAccessTokenSync();
    emit authenticationStatusChanged(!token.isEmpty());
}

void TraktCoreService::getUserProfile()
{
    apiRequest("/users/me?extended=full", "GET");
}

void TraktCoreService::getWatchedMovies()
{
    apiRequest("/sync/watched/movies", "GET");
}

void TraktCoreService::getWatchedShows()
{
    apiRequest("/sync/watched/shows", "GET");
}

void TraktCoreService::getWatchedMoviesSince(const QDateTime& since)
{
    // Use /sync/history/movies endpoint with start_at parameter
    QString endpoint = "/sync/history/movies";
    if (since.isValid()) {
        QString sinceStr = since.toString(Qt::ISODate);
        endpoint += "?start_at=" + QUrl::toPercentEncoding(sinceStr);
    }
    qDebug() << "[TraktCoreService] Fetching watched movies since:" << since.toString();
    apiRequest(endpoint, "GET");
}

void TraktCoreService::getWatchedShowsSince(const QDateTime& since)
{
    // Use /sync/history/episodes endpoint with start_at parameter
    QString endpoint = "/sync/history/episodes";
    if (since.isValid()) {
        QString sinceStr = since.toString(Qt::ISODate);
        endpoint += "?start_at=" + QUrl::toPercentEncoding(sinceStr);
    }
    qDebug() << "[TraktCoreService] Fetching watched shows since:" << since.toString();
    apiRequest(endpoint, "GET");
}

void TraktCoreService::syncWatchedMovies(bool forceFullSync)
{
    if (!m_database.isValid()) {
        emit syncError("watched_movies", "Database not initialized");
        return;
    }
    
    if (!m_syncDao) {
        m_syncDao = std::make_unique<SyncTrackingDao>();
    }
    
    QString syncType = "watched_movies";
    SyncTrackingRecord tracking = m_syncDao->getSyncTracking(std::string_view(syncType.toUtf8().constData(), syncType.toUtf8().size()));
    
    if (forceFullSync || !tracking.fullSyncCompleted) {
        qDebug() << "[TraktCoreService] Performing full sync for watched movies";
        // For full sync, use watched endpoint (no date filter)
        getWatchedMovies();
    } else {
        // Get last sync time and subtract buffer (1 hour) to ensure overlap
        QDateTime lastSync = tracking.lastSyncAt;
        QDateTime syncStart = lastSync.addSecs(-3600); // 1 hour buffer
        
        qDebug() << "[TraktCoreService] Performing incremental sync for watched movies since:" << syncStart.toString();
        getWatchedMoviesSince(syncStart);
    }
}

void TraktCoreService::syncWatchedShows(bool forceFullSync)
{
    if (!m_database.isValid()) {
        emit syncError("watched_shows", "Database not initialized");
        return;
    }
    
    if (!m_syncDao) {
        m_syncDao = std::make_unique<SyncTrackingDao>();
    }
    
    QString syncType = "watched_shows";
    SyncTrackingRecord tracking = m_syncDao->getSyncTracking(std::string_view(syncType.toUtf8().constData(), syncType.toUtf8().size()));
    
    if (forceFullSync || !tracking.fullSyncCompleted) {
        qDebug() << "[TraktCoreService] Performing full sync for watched shows";
        // For full sync, use watched endpoint (no date filter)
        getWatchedShows();
    } else {
        // Get last sync time and subtract buffer (1 hour) to ensure overlap
        QDateTime lastSync = tracking.lastSyncAt;
        QDateTime syncStart = lastSync.addSecs(-3600); // 1 hour buffer
        
        qDebug() << "[TraktCoreService] Performing incremental sync for watched shows since:" << syncStart.toString();
        getWatchedShowsSince(syncStart);
    }
}

bool TraktCoreService::isInitialSyncCompleted(const QString& syncType) const
{
    if (!m_syncDao || !m_database.isValid()) {
        return false;
    }
    
    SyncTrackingRecord tracking = m_syncDao->getSyncTracking(std::string_view(syncType.toUtf8().constData(), syncType.toUtf8().size()));
    return tracking.fullSyncCompleted;
}

QDateTime TraktCoreService::getLastSyncTime(const QString& syncType) const
{
    if (!m_syncDao || !m_database.isValid()) {
        return QDateTime();
    }
    
    SyncTrackingRecord tracking = m_syncDao->getSyncTracking(std::string_view(syncType.toUtf8().constData(), syncType.toUtf8().size()));
    return tracking.lastSyncAt;
}

void TraktCoreService::updateSyncTracking(const QString& syncType, bool fullSyncCompleted)
{
    if (!m_syncDao) {
        return;
    }
    
    QDateTime now = QDateTime::currentDateTime();
    (void)m_syncDao->upsertSyncTracking(std::string_view(syncType.toUtf8().constData(), syncType.toUtf8().size()), now, fullSyncCompleted);
    qDebug() << "[TraktCoreService] Updated sync tracking for" << syncType << "at" << now.toString();
}

int TraktCoreService::processAndStoreWatchedMovies(const QVariantList& movies)
{
    if (!m_watchHistoryDao || !m_syncDao) {
        qWarning() << "[TraktCoreService] Cannot process watched movies: DAOs not initialized";
        return 0;
    }
    
    int addedCount = 0;
    QDateTime lastSync = getLastSyncTime("watched_movies");
    QDateTime syncCutoff = lastSync.isValid() ? lastSync.addSecs(-3600) : QDateTime(); // 1 hour buffer
    
    for (const QVariant& movieVar : movies) {
        QVariantMap movieData = movieVar.toMap();
        
        // Watched endpoint structure: { movie: {...}, last_watched_at: "..." }
        QVariantMap movie = movieData["movie"].toMap();
        if (movie.isEmpty()) {
            continue; // Skip if no movie data
        }
        
        QVariantMap ids = movie["ids"].toMap();
        
        // Extract last_watched_at timestamp (API uses last_watched_at)
        QString watchedAtStr = movieData["last_watched_at"].toString();
        
        QDateTime watchedAt;
        if (!watchedAtStr.isEmpty()) {
            watchedAt = QDateTime::fromString(watchedAtStr, Qt::ISODate);
            if (!watchedAt.isValid()) {
                qWarning() << "[TraktCoreService] Invalid last_watched_at timestamp:" << watchedAtStr << ", using current time";
                watchedAt = QDateTime::currentDateTime();
            }
        } else {
            // If no timestamp, use a very old date to indicate it was watched but we don't know when
            watchedAt = QDateTime::fromMSecsSinceEpoch(0); // Epoch time as fallback
            qDebug() << "[TraktCoreService] Movie missing last_watched_at timestamp, using epoch time as fallback";
        }
        
        // For incremental sync, filter by timestamp (with buffer already applied in query)
        // Still check locally to be safe
        // Only filter if we have a valid timestamp (not epoch fallback)
        if (syncCutoff.isValid() && watchedAt > QDateTime::fromMSecsSinceEpoch(1000) && watchedAt < syncCutoff) {
            continue; // Skip items older than our cutoff (but not epoch fallbacks)
        }
        
        // Convert trakt data to watch history record
        WatchHistoryRecord record;
        record.contentId = QString::number(ids["trakt"].toInt());
        record.type = "movie";
        record.title = movie["title"].toString();
        record.year = movie["year"].toInt();
        record.imdbId = ids["imdb"].toString();
        record.tmdbId = ids.contains("tmdb") ? QString::number(ids["tmdb"].toInt()) : QString();
        record.watchedAt = watchedAt;
        record.progress = 1.0; // trakt watched items are 100% watched
        record.season = 0;
        record.episode = 0;
        
        qDebug() << "[TraktCoreService] Storing movie:" << record.title 
                 << "(" << record.year << ")" 
                 << "contentId:" << record.contentId 
                 << "imdbId:" << record.imdbId 
                 << "watchedAt:" << record.watchedAt.toString();
        
        // Use upsert to avoid duplicates
        if (m_watchHistoryDao->upsertWatchHistory(record)) {
            addedCount++;
            qDebug() << "[TraktCoreService] Successfully stored movie:" << record.title;
        } else {
            qWarning() << "[TraktCoreService] Failed to store movie:" << record.title;
        }
    }
    
    qDebug() << "[TraktCoreService] Processed" << addedCount << "watched movies";
    return addedCount;
}

int TraktCoreService::processAndStoreWatchedShows(const QVariantList& shows)
{
    if (!m_watchHistoryDao || !m_syncDao) {
        qWarning() << "[TraktCoreService] Cannot process watched shows: DAOs not initialized";
        return 0;
    }
    
    int addedCount = 0;
    QDateTime lastSync = getLastSyncTime("watched_shows");
    QDateTime syncCutoff = lastSync.isValid() ? lastSync.addSecs(-3600) : QDateTime(); // 1 hour buffer
    
    for (const QVariant& showVar : shows) {
        QVariantMap showData = showVar.toMap();
        
        // History endpoint returns episodes directly: { episode: {...}, show: {...}, watched_at: "..." }
        // Watched endpoint returns nested: { show: {...}, seasons: [{ episodes: [{ watched_at: "..." }] }] }
        // Handle both formats
        QVariantMap show;
        QVariantMap episode;
        QDateTime watchedAt;
        
        if (showData.contains("episode") && showData.contains("show")) {
            // History endpoint format (episodes) - uses last_watched_at
            episode = showData["episode"].toMap();
            show = showData["show"].toMap();
            QString watchedAtStr = showData["last_watched_at"].toString();
            if (!watchedAtStr.isEmpty()) {
                watchedAt = QDateTime::fromString(watchedAtStr, Qt::ISODate);
                if (!watchedAt.isValid()) {
                    qWarning() << "[TraktCoreService] Invalid last_watched_at timestamp:" << watchedAtStr << ", using epoch time";
                    watchedAt = QDateTime::fromMSecsSinceEpoch(0);
                }
            } else {
                // If no last_watched_at timestamp, use epoch time as fallback
                watchedAt = QDateTime::fromMSecsSinceEpoch(0);
                qDebug() << "[TraktCoreService] Episode missing last_watched_at timestamp, using epoch time as fallback";
            }
        } else if (showData.contains("seasons")) {
            // Watched endpoint format (nested seasons/episodes)
            QVariantMap showObj = showData["show"].toMap();
            QVariantMap showIds = showObj["ids"].toMap();
            
            QString showTitle = showObj["title"].toString();
            int showYear = showObj["year"].toInt();
            QString showImdbId = showIds["imdb"].toString();
            QString showTmdbId = showIds.contains("tmdb") ? QString::number(showIds["tmdb"].toInt()) : QString();
            QString showContentId = QString::number(showIds["trakt"].toInt());
            
            // Process seasons and episodes
            QVariantList seasons = showData["seasons"].toList();
            for (const QVariant& seasonVar : seasons) {
                QVariantMap seasonData = seasonVar.toMap();
                int seasonNum = seasonData["number"].toInt();
                
                QVariantList episodes = seasonData["episodes"].toList();
                for (const QVariant& episodeVar : episodes) {
                    QVariantMap episodeData = episodeVar.toMap();
                    
                    // Extract last_watched_at timestamp (API uses last_watched_at)
                    QString watchedAtStr = episodeData["last_watched_at"].toString();
                    
                    QDateTime epWatchedAt;
                    if (!watchedAtStr.isEmpty()) {
                        epWatchedAt = QDateTime::fromString(watchedAtStr, Qt::ISODate);
                        if (!epWatchedAt.isValid()) {
                            qWarning() << "[TraktCoreService] Invalid last_watched_at timestamp:" << watchedAtStr << ", using epoch time";
                            epWatchedAt = QDateTime::fromMSecsSinceEpoch(0);
                        }
                    } else {
                        // If no timestamp, use epoch time as fallback
                        epWatchedAt = QDateTime::fromMSecsSinceEpoch(0);
                        qDebug() << "[TraktCoreService] Episode missing last_watched_at timestamp, using epoch time as fallback";
                    }
                    
                    // For incremental sync, filter by timestamp
                    // Only filter if we have a valid timestamp (not epoch fallback)
                    if (syncCutoff.isValid() && epWatchedAt > QDateTime::fromMSecsSinceEpoch(1000) && epWatchedAt < syncCutoff) {
                        continue; // Skip items older than our cutoff (but not epoch fallbacks)
                    }
                    
                    int episodeNum = episodeData["number"].toInt();
                    QString episodeTitle = episodeData["title"].toString();
                    
                // Create watch history record for this episode
                WatchHistoryRecord record;
                record.contentId = showContentId;
                record.type = "tv";
                record.title = showTitle;
                record.year = showYear;
                record.imdbId = showImdbId;
                record.tmdbId = showTmdbId;
                record.watchedAt = epWatchedAt;
                record.progress = 1.0; // trakt watched episodes are 100% watched
                record.season = seasonNum;
                record.episode = episodeNum;
                record.episodeTitle = episodeTitle;
                
                qDebug() << "[TraktCoreService] Storing episode:" << showTitle 
                         << "S" << seasonNum << "E" << episodeNum 
                         << "(" << episodeTitle << ")"
                         << "contentId:" << showContentId 
                         << "watchedAt:" << epWatchedAt.toString();
                
                // Use upsert to avoid duplicates
                if (m_watchHistoryDao->upsertWatchHistory(record)) {
                    addedCount++;
                    qDebug() << "[TraktCoreService] Successfully stored episode:" << showTitle << "S" << seasonNum << "E" << episodeNum;
                } else {
                    qWarning() << "[TraktCoreService] Failed to store episode:" << showTitle << "S" << seasonNum << "E" << episodeNum;
                }
                }
            }
            continue; // Already processed, skip to next show
        } else {
            continue; // Unknown format
        }
        
        // Process history endpoint format (single episode)
        if (episode.isEmpty() || show.isEmpty()) {
            continue;
        }
        
        // Ensure we have a valid watchedAt (should be set above, but double-check)
        if (!watchedAt.isValid()) {
            watchedAt = QDateTime::fromMSecsSinceEpoch(0);
        }
        
        // For incremental sync, filter by timestamp
        // Only filter if we have a valid timestamp (not epoch fallback)
        if (syncCutoff.isValid() && watchedAt > QDateTime::fromMSecsSinceEpoch(1000) && watchedAt < syncCutoff) {
            continue; // Skip items older than our cutoff (but not epoch fallbacks)
        }
        
        QVariantMap showIds = show["ids"].toMap();
        QVariantMap episodeIds = episode["ids"].toMap();
        
        QString showTitle = show["title"].toString();
        int showYear = show["year"].toInt();
        QString showImdbId = showIds["imdb"].toString();
        QString showTmdbId = showIds.contains("tmdb") ? QString::number(showIds["tmdb"].toInt()) : QString();
        QString showContentId = QString::number(showIds["trakt"].toInt());
        
        int seasonNum = episode["season"].toInt();
        int episodeNum = episode["number"].toInt();
        QString episodeTitle = episode["title"].toString();
        
        // Create watch history record for this episode
        WatchHistoryRecord record;
        record.contentId = showContentId;
        record.type = "tv";
        record.title = showTitle;
        record.year = showYear;
        record.imdbId = showImdbId;
        record.tmdbId = showTmdbId;
        record.watchedAt = watchedAt;
        record.progress = 1.0; // trakt watched episodes are 100% watched
        record.season = seasonNum;
        record.episode = episodeNum;
        record.episodeTitle = episodeTitle;
        
        // Use upsert to avoid duplicates
        if (m_watchHistoryDao->upsertWatchHistory(record)) {
            addedCount++;
        }
    }
    
    qDebug() << "[TraktCoreService] Processed" << addedCount << "watched show episodes";
    return addedCount;
}

void TraktCoreService::getWatchlistMoviesWithImages()
{
    apiRequest("/sync/watchlist/movies?extended=images", "GET");
}

void TraktCoreService::getWatchlistShowsWithImages()
{
    apiRequest("/sync/watchlist/shows?extended=images", "GET");
}

void TraktCoreService::getCollectionMoviesWithImages()
{
    apiRequest("/sync/collection/movies?extended=images", "GET");
}

void TraktCoreService::getCollectionShowsWithImages()
{
    apiRequest("/sync/collection/shows?extended=images", "GET");
}

void TraktCoreService::getRatingsWithImages(const QString& type)
{
    QString endpoint = type.isEmpty() ? "/sync/ratings?extended=images" 
                                      : QString("/sync/ratings/%1?extended=images").arg(type);
    apiRequest(endpoint, "GET");
}

void TraktCoreService::getPlaybackProgressWithImages(const QString& type)
{
    QString endpoint = type.isEmpty() ? "/sync/playback?extended=images"
                                      : QString("/sync/playback/%1?extended=images").arg(type);
    apiRequest(endpoint, "GET");
}

void TraktCoreService::getTraktIdFromImdbId(const QString& imdbId, const QString& type)
{
    QString cleanImdbId = imdbId.startsWith("tt") ? imdbId.mid(2) : imdbId;
    QString endpoint = QString("/search/%1?id_type=imdb&id=%2").arg(type, cleanImdbId);
    apiRequest(endpoint, "GET");
}

void TraktCoreService::getTraktIdFromTmdbId(int tmdbId, const QString& type)
{
    QString endpoint = QString("/search/%1?id_type=tmdb&id=%2").arg(type).arg(tmdbId);
    apiRequest(endpoint, "GET");
}

void TraktCoreService::onApiReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    QString endpoint = reply->property("endpoint").toString();
    
    if (reply->error() != QNetworkReply::NoError) {
        // Emit sync error for sync endpoints
        if (endpoint.contains("/sync/watched/movies") || endpoint.contains("/sync/history/movies")) {
            QString errorMsg = reply->errorString();
            qWarning() << "[TraktCoreService] Error fetching watched movies:" << errorMsg;
            emit syncError("watched_movies", errorMsg);
        } else if (endpoint.contains("/sync/watched/shows") || endpoint.contains("/sync/history/episodes")) {
            QString errorMsg = reply->errorString();
            qWarning() << "[TraktCoreService] Error fetching watched shows:" << errorMsg;
            emit syncError("watched_shows", errorMsg);
        } else {
            handleError(reply, endpoint);
        }
        reply->deleteLater();
        return;
    }
    
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    
    // Handle "No Content" responses
    if (statusCode == 204 || statusCode == 205) {
        reply->deleteLater();
        return;
    }
    
    QByteArray data = reply->readAll();
    if (data.isEmpty()) {
        qDebug() << "[TraktCoreService] Empty response for" << endpoint;
        
        // Handle empty responses for sync endpoints - still emit completion signals
        if (endpoint.contains("/sync/watched/movies") || endpoint.contains("/sync/history/movies")) {
            qDebug() << "[TraktCoreService] Empty watched movies response, emitting completion";
            updateSyncTracking("watched_movies", true);
            emit watchedMoviesSynced(0, 0);
        } else if (endpoint.contains("/sync/watched/shows") || endpoint.contains("/sync/history/episodes")) {
            qDebug() << "[TraktCoreService] Empty watched shows response, emitting completion";
            updateSyncTracking("watched_shows", true);
            emit watchedShowsSynced(0, 0);
        }
        
        reply->deleteLater();
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        qWarning() << "[TraktCoreService] Invalid JSON response for" << endpoint;
        
        // Emit sync error for sync endpoints
        if (endpoint.contains("/sync/watched/movies") || endpoint.contains("/sync/history/movies")) {
            emit syncError("watched_movies", "Invalid JSON response");
        } else if (endpoint.contains("/sync/watched/shows") || endpoint.contains("/sync/history/episodes")) {
            emit syncError("watched_shows", "Invalid JSON response");
        } else {
            emit error("Invalid JSON response");
        }
        
        reply->deleteLater();
        return;
    }
    
    // Cache successful GET responses
    QString method = reply->property("method").toString();
    if (method == "GET" && statusCode == 200) {
        QString cacheKey = getCacheKey(endpoint);
        int ttlSeconds = getTtlForEndpoint(endpoint);
        if (doc.isObject()) {
            cacheResponse(cacheKey, doc.object(), ttlSeconds);
        } else if (doc.isArray()) {
            // Convert array to object for caching
            QJsonObject cacheObj;
            cacheObj["_array"] = doc.array();
            cacheResponse(cacheKey, cacheObj, ttlSeconds);
        }
    }
    
    // Parse response based on endpoint
    if (endpoint.contains("/users/me")) {
        emit userProfileFetched(doc.object());
    } else if (endpoint.contains("/sync/watched/movies") || endpoint.contains("/sync/history/movies")) {
        QVariantList movies;
        QJsonArray arr = doc.array();
        for (const QJsonValue& val : arr) {
            movies.append(val.toObject().toVariantMap());
        }
        
        qDebug() << "[TraktCoreService] ===== MOVIES SYNC =====";
        qDebug() << "[TraktCoreService] Received" << movies.size() << "watched movies from API";
        
        // Process and store watched movies
        int addedCount = processAndStoreWatchedMovies(movies);
        
        // Update sync tracking
        updateSyncTracking("watched_movies", true);
        
        qDebug() << "[TraktCoreService] Movies sync complete - received" << movies.size() << "from API, stored" << addedCount << "new items";
        qDebug() << "[TraktCoreService] ===== END MOVIES SYNC =====";
        emit watchedMoviesFetched(movies);
        emit watchedMoviesSynced(addedCount, 0);
    } else if (endpoint.contains("/sync/watched/shows") || endpoint.contains("/sync/history/episodes")) {
        QVariantList shows;
        QJsonArray arr = doc.array();
        for (const QJsonValue& val : arr) {
            shows.append(val.toObject().toVariantMap());
        }
        
        qDebug() << "[TraktCoreService] ===== SHOWS SYNC =====";
        qDebug() << "[TraktCoreService] Received" << shows.size() << "watched shows/episodes from API";
        
        // Process and store watched shows
        int addedCount = processAndStoreWatchedShows(shows);
        
        // Update sync tracking
        updateSyncTracking("watched_shows", true);
        
        qDebug() << "[TraktCoreService] Shows sync complete - received" << shows.size() << "from API, stored" << addedCount << "new episodes";
        qDebug() << "[TraktCoreService] ===== END SHOWS SYNC =====";
        emit watchedShowsFetched(shows);
        emit watchedShowsSynced(addedCount, 0);
    } else if (endpoint.contains("/sync/watchlist/movies")) {
        QVariantList movies;
        QJsonArray arr = doc.array();
        for (const QJsonValue& val : arr) {
            movies.append(val.toObject().toVariantMap());
        }
        emit watchlistMoviesFetched(movies);
    } else if (endpoint.contains("/sync/watchlist/shows")) {
        QVariantList shows;
        QJsonArray arr = doc.array();
        for (const QJsonValue& val : arr) {
            shows.append(val.toObject().toVariantMap());
        }
        emit watchlistShowsFetched(shows);
    } else if (endpoint.contains("/sync/collection/movies")) {
        QVariantList movies;
        QJsonArray arr = doc.array();
        for (const QJsonValue& val : arr) {
            movies.append(val.toObject().toVariantMap());
        }
        emit collectionMoviesFetched(movies);
    } else if (endpoint.contains("/sync/collection/shows")) {
        QVariantList shows;
        QJsonArray arr = doc.array();
        for (const QJsonValue& val : arr) {
            shows.append(val.toObject().toVariantMap());
        }
        emit collectionShowsFetched(shows);
    } else if (endpoint.contains("/sync/ratings")) {
        QVariantList ratings;
        QJsonArray arr = doc.array();
        for (const QJsonValue& val : arr) {
            ratings.append(val.toObject().toVariantMap());
        }
        emit ratingsFetched(ratings);
    } else if (endpoint.contains("/sync/playback")) {
        QVariantList progress;
        QJsonArray arr = doc.array();
        for (const QJsonValue& val : arr) {
            progress.append(val.toObject().toVariantMap());
        }
        emit playbackProgressFetched(progress);
    } else if (endpoint.contains("/search/")) {
        QJsonArray arr = doc.array();
        if (!arr.isEmpty()) {
            QJsonObject first = arr[0].toObject();
            QString type = endpoint.contains("/search/movie") ? "movie" : "show";
            QJsonObject item = first[type].toObject();
            QJsonObject ids = item["ids"].toObject();
            int traktId = ids["trakt"].toString().toInt();
            QString imdbId = reply->property("imdbId").toString();
            if (!imdbId.isEmpty()) {
                emit traktIdFound(imdbId, traktId);
            }
        }
    }

    reply->deleteLater();
}

void TraktCoreService::clearCache()
{
    m_cache.clear();
    qDebug() << "[TraktCoreService] Cache cleared";
}

void TraktCoreService::clearCacheForEndpoint(const QString& endpoint)
{
    QStringList keysToRemove;
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        if (it.key().startsWith(endpoint)) {
            keysToRemove << it.key();
        }
    }
    for (const QString& key : keysToRemove) {
        m_cache.remove(key);
    }
    qDebug() << "[TraktCoreService] Cleared cache for endpoint:" << endpoint << "(" << keysToRemove.size() << " entries)";
}

void TraktCoreService::clearSyncTracking(const QString& syncType)
{
    if (!m_syncDao) {
        qWarning() << "[TraktCoreService] Cannot clear sync tracking: DAO not initialized";
        return;
    }
    
    (void)m_syncDao->deleteSyncTracking(std::string_view(syncType.toUtf8().constData(), syncType.toUtf8().size()));
    qDebug() << "[TraktCoreService] Cleared sync tracking for:" << syncType;
}

void TraktCoreService::resyncWatchedHistory()
{
    if (!m_database.isValid()) {
        emit syncError("resync", "Database not initialized");
        return;
    }
    
    if (!m_watchHistoryDao || !m_syncDao) {
        qWarning() << "[TraktCoreService] Cannot resync: DAOs not initialized";
        emit syncError("resync", "DAOs not initialized");
        return;
    }
    
    qDebug() << "[TraktCoreService] Starting full resync - clearing local watch history and sync tracking";
    
    // Clear watch history
    if (!m_watchHistoryDao->clearWatchHistory()) {
        qWarning() << "[TraktCoreService] Failed to clear watch history";
        emit syncError("resync", "Failed to clear watch history");
        return;
    }
    
    // Clear sync tracking for both movies and shows
    (void)m_syncDao->deleteSyncTracking(std::string_view("watched_movies"));
    (void)m_syncDao->deleteSyncTracking(std::string_view("watched_shows"));
    
    qDebug() << "[TraktCoreService] Cleared watch history and sync tracking, starting full sync";
    
    // Trigger full sync for both movies and shows
    syncWatchedMovies(true);  // forceFullSync = true
    syncWatchedShows(true);   // forceFullSync = true
}

