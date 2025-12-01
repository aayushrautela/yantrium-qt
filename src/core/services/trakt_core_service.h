#ifndef TRAKT_CORE_SERVICE_H
#define TRAKT_CORE_SERVICE_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QDateTime>
#include <QMap>
#include <QSet>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QQueue>
#include <QSqlDatabase>
#include "../models/trakt_models.h"
#include "../database/trakt_auth_dao.h"

class TraktCoreService : public QObject
{
    Q_OBJECT

public:
    static TraktCoreService& instance();
    
    void setDatabase(QSqlDatabase database);
    void initializeAuth();
    void reloadAuth();
    
    Q_INVOKABLE void checkAuthentication();
    Q_INVOKABLE void getUserProfile();
    Q_INVOKABLE void getWatchedMovies();
    Q_INVOKABLE void getWatchedShows();
    Q_INVOKABLE void getWatchlistMoviesWithImages();
    Q_INVOKABLE void getWatchlistShowsWithImages();
    Q_INVOKABLE void getCollectionMoviesWithImages();
    Q_INVOKABLE void getCollectionShowsWithImages();
    Q_INVOKABLE void getRatingsWithImages(const QString& type = QString());
    Q_INVOKABLE void getPlaybackProgressWithImages(const QString& type = QString());
    Q_INVOKABLE void getTraktIdFromImdbId(const QString& imdbId, const QString& type);
    Q_INVOKABLE void getTraktIdFromTmdbId(int tmdbId, const QString& type);
    
    void setCompletionThreshold(int threshold);
    int completionThreshold() const { return m_completionThreshold; }
    
    // Logout (public for use by auth service)
    void logout();
    
    // Internal API request method (used by other services)
    void apiRequest(const QString& endpoint, const QString& method = "GET", 
                   const QJsonObject& data = QJsonObject(), QObject* receiver = nullptr,
                   const char* slot = nullptr);

signals:
    void authenticationStatusChanged(bool authenticated);
    void userProfileFetched(const QJsonObject& user);
    void watchedMoviesFetched(const QVariantList& movies);
    void watchedShowsFetched(const QVariantList& shows);
    void watchlistMoviesFetched(const QVariantList& movies);
    void watchlistShowsFetched(const QVariantList& shows);
    void collectionMoviesFetched(const QVariantList& movies);
    void collectionShowsFetched(const QVariantList& shows);
    void ratingsFetched(const QVariantList& ratings);
    void playbackProgressFetched(const QVariantList& progress);
    void traktIdFound(const QString& imdbId, int traktId);
    void error(const QString& message);

private slots:
    void onApiReplyFinished();
    void processRequestQueue();
    void cleanupOldData();

private:
    explicit TraktCoreService(QObject* parent = nullptr);
    ~TraktCoreService();
    Q_DISABLE_COPY(TraktCoreService)
    
    QString getAccessTokenSync();
    void refreshAccessToken();
    void saveTokens(const QString& accessToken, const QString& refreshToken, int expiresIn);
    
    QUrl buildUrl(const QString& endpoint);
    void handleError(QNetworkReply* reply, const QString& context);
    QString getContentKeyFromPayload(const QJsonObject& payload);
    bool isRecentlyScrobbled(const QString& contentKey);
    
    QNetworkAccessManager* m_networkManager;
    QSqlDatabase m_database;
    TraktAuthDao* m_authDao;
    
    // Authentication state
    QString m_accessToken;
    QString m_refreshToken;
    qint64 m_tokenExpiry;  // milliseconds since epoch
    bool m_isInitialized;
    
    // Rate limiting
    qint64 m_lastApiCall;  // milliseconds since epoch
    static constexpr int MIN_API_INTERVAL_MS = 500;  // 500ms minimum interval
    
    // Request queue
    struct QueuedRequest {
        QString endpoint;
        QString method;
        QJsonObject data;
        QObject* receiver;
        const char* slot;
    };
    QQueue<QueuedRequest> m_requestQueue;
    QTimer* m_queueTimer;
    bool m_isProcessingQueue;
    
    // Deduplication
    QSet<QString> m_scrobbledItems;
    QMap<QString, qint64> m_scrobbledTimestamps;
    static constexpr int SCROBBLE_EXPIRY_MS = 46 * 60 * 1000;  // 46 minutes
    
    // Debouncing
    QSet<QString> m_currentlyWatching;
    QMap<QString, qint64> m_lastSyncTimes;
    static constexpr int SYNC_DEBOUNCE_MS = 5000;  // 5 seconds
    
    QMap<QString, qint64> m_lastStopCalls;
    static constexpr int STOP_DEBOUNCE_MS = 1000;  // 1 second
    
    // Completion threshold
    int m_completionThreshold;  // Default 81% (more than 80% is considered watched)
    
    // Cleanup timer
    QTimer* m_cleanupTimer;
};

#endif // TRAKT_CORE_SERVICE_H

