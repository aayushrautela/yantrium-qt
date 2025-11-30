#ifndef TRAKT_WATCHLIST_SERVICE_H
#define TRAKT_WATCHLIST_SERVICE_H

#include <QObject>
#include <QString>
#include <QVariantList>
#include "../services/trakt_core_service.h"

class TraktWatchlistService : public QObject
{
    Q_OBJECT

public:
    explicit TraktWatchlistService(QObject* parent = nullptr);
    
    // Watchlist methods
    Q_INVOKABLE void getWatchlistMoviesWithImages();
    Q_INVOKABLE void getWatchlistShowsWithImages();
    Q_INVOKABLE void addToWatchlist(const QString& type, const QString& imdbId);
    Q_INVOKABLE void removeFromWatchlist(const QString& type, const QString& imdbId);
    Q_INVOKABLE void isInWatchlist(const QString& imdbId, const QString& type);
    
    // Collection methods
    Q_INVOKABLE void getCollectionMoviesWithImages();
    Q_INVOKABLE void getCollectionShowsWithImages();
    Q_INVOKABLE void addToCollection(const QString& type, const QString& imdbId);
    Q_INVOKABLE void removeFromCollection(const QString& type, const QString& imdbId);
    Q_INVOKABLE void isInCollection(const QString& imdbId, const QString& type);

signals:
    void watchlistMoviesFetched(const QVariantList& movies);
    void watchlistShowsFetched(const QVariantList& shows);
    void collectionMoviesFetched(const QVariantList& movies);
    void collectionShowsFetched(const QVariantList& shows);
    void watchlistItemAdded(bool success);
    void watchlistItemRemoved(bool success);
    void collectionItemAdded(bool success);
    void collectionItemRemoved(bool success);
    void isInWatchlistResult(bool inWatchlist);
    void isInCollectionResult(bool inCollection);
    void error(const QString& message);

private slots:
    void onWatchlistMoviesFetched(const QVariantList& movies);
    void onWatchlistShowsFetched(const QVariantList& shows);
    void onCollectionMoviesFetched(const QVariantList& movies);
    void onCollectionShowsFetched(const QVariantList& shows);
    void onWatchlistItemAdded();
    void onWatchlistItemRemoved();
    void onCollectionItemAdded();
    void onCollectionItemRemoved();

private:
    TraktCoreService& m_coreService;
    QVariantList m_watchlistMovies;
    QVariantList m_watchlistShows;
    QVariantList m_collectionMovies;
    QVariantList m_collectionShows;
    
    QString ensureImdbPrefix(const QString& imdbId) const;
};

#endif // TRAKT_WATCHLIST_SERVICE_H

