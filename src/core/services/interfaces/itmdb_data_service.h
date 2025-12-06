#ifndef ITMDB_DATA_SERVICE_H
#define ITMDB_DATA_SERVICE_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantList>

/**
 * @brief Interface for TMDB data service operations
 * 
 * Pure abstract interface - no QObject inheritance to avoid diamond inheritance.
 * Signals are defined in concrete implementations.
 */
class ITmdbDataService
{
public:
    virtual ~ITmdbDataService() = default;
    
    virtual void getTmdbIdFromImdb(const QString& imdbId) = 0;
    virtual void getMovieMetadata(int tmdbId) = 0;
    virtual void getTvMetadata(int tmdbId) = 0;
    virtual void getCastAndCrew(int tmdbId, const QString& type) = 0;
    virtual void getSimilarMovies(int tmdbId) = 0;
    virtual void getSimilarTv(int tmdbId) = 0;
    virtual void searchMovies(const QString& query, int page = 1) = 0;
    virtual void searchTv(const QString& query, int page = 1) = 0;
    virtual void getTvSeasonDetails(int tmdbId, int seasonNumber) = 0;
    
    // Cache management
    virtual void clearCache() = 0;
    virtual void clearCacheForId(int tmdbId, const QString& type) = 0;
    [[nodiscard]] virtual int getCacheSize() const = 0;
};

#endif // ITMDB_DATA_SERVICE_H

