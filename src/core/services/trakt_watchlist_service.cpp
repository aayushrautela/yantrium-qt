#include "trakt_watchlist_service.h"
#include "../database/database_manager.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>

TraktWatchlistService::TraktWatchlistService(QObject* parent)
    : QObject(parent)
    , m_coreService(TraktCoreService::instance())
{
    DatabaseManager& dbManager = DatabaseManager::instance();
    if (dbManager.isInitialized()) {
        m_coreService.setDatabase(dbManager.database());
        m_coreService.initializeAuth();
    }
    
    connect(&m_coreService, &TraktCoreService::watchlistMoviesFetched,
            this, &TraktWatchlistService::onWatchlistMoviesFetched);
    connect(&m_coreService, &TraktCoreService::watchlistShowsFetched,
            this, &TraktWatchlistService::onWatchlistShowsFetched);
    connect(&m_coreService, &TraktCoreService::collectionMoviesFetched,
            this, &TraktWatchlistService::onCollectionMoviesFetched);
    connect(&m_coreService, &TraktCoreService::collectionShowsFetched,
            this, &TraktWatchlistService::onCollectionShowsFetched);
}

QString TraktWatchlistService::ensureImdbPrefix(const QString& imdbId) const
{
    return imdbId.startsWith("tt") ? imdbId : "tt" + imdbId;
}

void TraktWatchlistService::getWatchlistMoviesWithImages()
{
    m_coreService.getWatchlistMoviesWithImages();
}

void TraktWatchlistService::getWatchlistShowsWithImages()
{
    m_coreService.getWatchlistShowsWithImages();
}

void TraktWatchlistService::addToWatchlist(const QString& type, const QString& imdbId)
{
    if (imdbId.trimmed().isEmpty()) {
        emit error("IMDb ID is required");
        return;
    }
    
    if (type != "movie" && type != "show") {
        emit error("Type must be either 'movie' or 'show'");
        return;
    }
    
    QString cleanImdbId = ensureImdbPrefix(imdbId);
    QJsonObject payload;
    QJsonArray items;
    QJsonObject item;
    QJsonObject ids;
    ids["imdb"] = cleanImdbId;
    item["ids"] = ids;
    items.append(item);
    
    if (type == "movie") {
        payload["movies"] = items;
    } else {
        payload["shows"] = items;
    }
    
    m_coreService.apiRequest("/sync/watchlist", "POST", payload, this, SLOT(onWatchlistItemAdded()));
}

void TraktWatchlistService::removeFromWatchlist(const QString& type, const QString& imdbId)
{
    if (imdbId.trimmed().isEmpty()) {
        emit error("IMDb ID is required");
        return;
    }
    
    if (type != "movie" && type != "show") {
        emit error("Type must be either 'movie' or 'show'");
        return;
    }
    
    QString cleanImdbId = ensureImdbPrefix(imdbId);
    QJsonObject payload;
    QJsonArray items;
    QJsonObject item;
    QJsonObject ids;
    ids["imdb"] = cleanImdbId;
    item["ids"] = ids;
    items.append(item);
    
    if (type == "movie") {
        payload["movies"] = items;
    } else {
        payload["shows"] = items;
    }
    
    m_coreService.apiRequest("/sync/watchlist/remove", "POST", payload, this, SLOT(onWatchlistItemRemoved()));
}

void TraktWatchlistService::isInWatchlist(const QString& imdbId, const QString& type)
{
    QString cleanImdbId = ensureImdbPrefix(imdbId);
    
    if (type == "movie") {
        if (m_watchlistMovies.isEmpty()) {
            getWatchlistMoviesWithImages();
            // Wait for results - this is async, so we need to check after fetch
            // For now, emit false and let the caller check again after fetching
            emit isInWatchlistResult(false);
            return;
        }
        
        for (const QVariant& item : m_watchlistMovies) {
            QJsonObject movieObj = item.toJsonObject();
            QJsonObject movie = movieObj["movie"].toObject();
            QJsonObject ids = movie["ids"].toObject();
            if (ids["imdb"].toString() == cleanImdbId) {
                emit isInWatchlistResult(true);
                return;
            }
        }
    } else {
        if (m_watchlistShows.isEmpty()) {
            getWatchlistShowsWithImages();
            emit isInWatchlistResult(false);
            return;
        }
        
        for (const QVariant& item : m_watchlistShows) {
            QJsonObject showObj = item.toJsonObject();
            QJsonObject show = showObj["show"].toObject();
            QJsonObject ids = show["ids"].toObject();
            if (ids["imdb"].toString() == cleanImdbId) {
                emit isInWatchlistResult(true);
                return;
            }
        }
    }
    
    emit isInWatchlistResult(false);
}

void TraktWatchlistService::getCollectionMoviesWithImages()
{
    m_coreService.getCollectionMoviesWithImages();
}

void TraktWatchlistService::getCollectionShowsWithImages()
{
    m_coreService.getCollectionShowsWithImages();
}

void TraktWatchlistService::addToCollection(const QString& type, const QString& imdbId)
{
    if (imdbId.trimmed().isEmpty()) {
        emit error("IMDb ID is required");
        return;
    }
    
    if (type != "movie" && type != "show") {
        emit error("Type must be either 'movie' or 'show'");
        return;
    }
    
    QString cleanImdbId = ensureImdbPrefix(imdbId);
    QJsonObject payload;
    QJsonArray items;
    QJsonObject item;
    QJsonObject ids;
    ids["imdb"] = cleanImdbId;
    item["ids"] = ids;
    items.append(item);
    
    if (type == "movie") {
        payload["movies"] = items;
    } else {
        payload["shows"] = items;
    }
    
    m_coreService.apiRequest("/sync/collection", "POST", payload, this, SLOT(onCollectionItemAdded()));
}

void TraktWatchlistService::removeFromCollection(const QString& type, const QString& imdbId)
{
    if (imdbId.trimmed().isEmpty()) {
        emit error("IMDb ID is required");
        return;
    }
    
    if (type != "movie" && type != "show") {
        emit error("Type must be either 'movie' or 'show'");
        return;
    }
    
    QString cleanImdbId = ensureImdbPrefix(imdbId);
    QJsonObject payload;
    QJsonArray items;
    QJsonObject item;
    QJsonObject ids;
    ids["imdb"] = cleanImdbId;
    item["ids"] = ids;
    items.append(item);
    
    if (type == "movie") {
        payload["movies"] = items;
    } else {
        payload["shows"] = items;
    }
    
    m_coreService.apiRequest("/sync/collection/remove", "POST", payload, this, SLOT(onCollectionItemRemoved()));
}

void TraktWatchlistService::isInCollection(const QString& imdbId, const QString& type)
{
    QString cleanImdbId = ensureImdbPrefix(imdbId);
    
    if (type == "movie") {
        if (m_collectionMovies.isEmpty()) {
            getCollectionMoviesWithImages();
            emit isInCollectionResult(false);
            return;
        }
        
        for (const QVariant& item : m_collectionMovies) {
            QJsonObject movieObj = item.toJsonObject();
            QJsonObject movie = movieObj["movie"].toObject();
            QJsonObject ids = movie["ids"].toObject();
            if (ids["imdb"].toString() == cleanImdbId) {
                emit isInCollectionResult(true);
                return;
            }
        }
    } else {
        if (m_collectionShows.isEmpty()) {
            getCollectionShowsWithImages();
            emit isInCollectionResult(false);
            return;
        }
        
        for (const QVariant& item : m_collectionShows) {
            QJsonObject showObj = item.toJsonObject();
            QJsonObject show = showObj["show"].toObject();
            QJsonObject ids = show["ids"].toObject();
            if (ids["imdb"].toString() == cleanImdbId) {
                emit isInCollectionResult(true);
                return;
            }
        }
    }
    
    emit isInCollectionResult(false);
}

void TraktWatchlistService::onWatchlistMoviesFetched(const QVariantList& movies)
{
    m_watchlistMovies = movies;
    emit watchlistMoviesFetched(movies);
}

void TraktWatchlistService::onWatchlistShowsFetched(const QVariantList& shows)
{
    m_watchlistShows = shows;
    emit watchlistShowsFetched(shows);
}

void TraktWatchlistService::onCollectionMoviesFetched(const QVariantList& movies)
{
    m_collectionMovies = movies;
    emit collectionMoviesFetched(movies);
}

void TraktWatchlistService::onCollectionShowsFetched(const QVariantList& shows)
{
    m_collectionShows = shows;
    emit collectionShowsFetched(shows);
}

// Add missing slot implementations
void TraktWatchlistService::onWatchlistItemAdded()
{
    emit watchlistItemAdded(true);
}

void TraktWatchlistService::onWatchlistItemRemoved()
{
    emit watchlistItemRemoved(true);
}

void TraktWatchlistService::onCollectionItemAdded()
{
    emit collectionItemAdded(true);
}

void TraktWatchlistService::onCollectionItemRemoved()
{
    emit collectionItemRemoved(true);
}



