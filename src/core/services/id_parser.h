#ifndef ID_PARSER_H
#define ID_PARSER_H

#include <QString>

/// Utility for parsing content IDs from addons
class IdParser
{
public:
    /// Extract TMDB ID from various ID formats
    /// Supports: "tmdb:123", "tt1234567" (IMDB), numeric strings
    static int extractTmdbId(const QString& contentId);
    
    /// Check if ID is an IMDB ID
    static bool isImdbId(const QString& contentId);
    
    /// Check if ID is a TMDB ID
    static bool isTmdbId(const QString& contentId);
};

#endif // ID_PARSER_H

