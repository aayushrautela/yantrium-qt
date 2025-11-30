#ifndef TMDB_DATA_EXTRACTOR_H
#define TMDB_DATA_EXTRACTOR_H

#include <QString>
#include <QJsonObject>

/// Utility class for extracting structured data from TMDB API responses
class TmdbDataExtractor
{
public:
    /// Extract maturity rating from TMDB data
    /// For movies: extracts from release_dates.results[US].release_dates[].certification
    /// For TV shows: extracts from content_ratings.results[US].rating
    static QString extractMaturityRating(const QJsonObject& tmdbData, const QString& type);
    
    /// Get the full name/description for a maturity rating
    /// Maps rating codes to their full names based on MPAA (movies) and TV Parental Guidelines (series)
    static QString getMaturityRatingName(const QString& rating, const QString& type);
    
    /// Extract cast and crew data from TMDB response
    /// Returns a JSON object with 'cast' and 'crew' arrays
    static QJsonObject extractCastAndCrew(const QJsonObject& tmdbData);
    
    /// Extract production information (companies, countries, languages)
    static QJsonObject extractProductionInfo(const QJsonObject& tmdbData, const QString& type);
    
    /// Extract release information (dates, status)
    static QJsonObject extractReleaseInfo(const QJsonObject& tmdbData, const QString& type);
    
    /// Extract additional metadata (budget, revenue, tagline, etc.)
    static QJsonObject extractAdditionalMetadata(const QJsonObject& tmdbData, const QString& type);
    
    /// Extract poster URL from TMDB data
    static QString extractPosterUrl(const QJsonObject& tmdbData, const QString& imageBaseUrl = QString());
    
    /// Extract backdrop URL from TMDB data
    static QString extractBackdropUrl(const QJsonObject& tmdbData, const QString& imageBaseUrl = QString());
    
    /// Extract logo URL from TMDB data (prefers US/English logos)
    static QString extractLogoUrl(const QJsonObject& tmdbData, const QString& imageBaseUrl = QString());
};

#endif // TMDB_DATA_EXTRACTOR_H

