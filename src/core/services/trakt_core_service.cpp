#include "trakt_core_service.h"
#include "configuration.h"
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include <QTimer>
#include <QElapsedTimer>
#include <QEventLoop>
#include <cmath>

TraktCoreService& TraktCoreService::instance()
{
    static TraktCoreService instance;
    return instance;
}

TraktCoreService::TraktCoreService(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_authDao(nullptr)
    , m_tokenExpiry(0)
    , m_isInitialized(false)
    , m_lastApiCall(0)
    , m_queueTimer(new QTimer(this))
    , m_isProcessingQueue(false)
    , m_completionThreshold(81)  // More than 80% (>80%) is considered watched
    , m_cleanupTimer(new QTimer(this))
{
    m_queueTimer->setSingleShot(true);
    m_queueTimer->setInterval(MIN_API_INTERVAL_MS);
    connect(m_queueTimer, &QTimer::timeout, this, &TraktCoreService::processRequestQueue);
    
    m_cleanupTimer->setInterval(60000);  // Cleanup every minute
    connect(m_cleanupTimer, &QTimer::timeout, this, &TraktCoreService::cleanupOldData);
    m_cleanupTimer->start();
}

TraktCoreService::~TraktCoreService()
{
}

void TraktCoreService::setDatabase(QSqlDatabase database)
{
    if (m_database.isValid()) {
        qDebug() << "[TraktCoreService] Database already set";
        return;
    }
    m_database = database;
    m_authDao = new TraktAuthDao(database);
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
        
        m_authDao->upsertTraktAuth(record);
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
        m_authDao->deleteTraktAuth();
        qDebug() << "[TraktCoreService] User logged out successfully";
    }
}

QUrl TraktCoreService::buildUrl(const QString& endpoint)
{
    Configuration& config = Configuration::instance();
    return QUrl(config.traktBaseUrl() + endpoint);
}

void TraktCoreService::apiRequest(const QString& endpoint, const QString& method,
                                  const QJsonObject& data, QObject* receiver, const char* slot)
{
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
        handleError(reply, endpoint);
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
        reply->deleteLater();
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        qWarning() << "[TraktCoreService] Invalid JSON response for" << endpoint;
        emit error("Invalid JSON response");
        reply->deleteLater();
        return;
    }
    
    // Parse response based on endpoint
    if (endpoint.contains("/users/me")) {
        emit userProfileFetched(doc.object());
    } else if (endpoint.contains("/sync/watched/movies")) {
        QVariantList movies;
        QJsonArray arr = doc.array();
        for (const QJsonValue& val : arr) {
            movies.append(val.toObject().toVariantMap());
        }
        emit watchedMoviesFetched(movies);
    } else if (endpoint.contains("/sync/watched/shows")) {
        QVariantList shows;
        QJsonArray arr = doc.array();
        for (const QJsonValue& val : arr) {
            shows.append(val.toObject().toVariantMap());
        }
        emit watchedShowsFetched(shows);
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

