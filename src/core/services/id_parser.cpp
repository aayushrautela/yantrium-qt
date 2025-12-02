#include "id_parser.h"
#include <QDebug>

int IdParser::extractTmdbId(const QString& contentId)
{
    // Handle tmdb:123 format
    if (contentId.startsWith("tmdb:")) {
        QString id = contentId.mid(5);
        bool ok;
        int result = id.toInt(&ok);
        if (ok) {
            return result;
        }
        return 0;
    }
    
    // Handle numeric string (assume TMDB ID)
    bool ok;
    int numericId = contentId.toInt(&ok);
    if (ok && numericId > 0) {
        return numericId;
    }
    
    // Handle IMDB ID (tt1234567) - return 0, will need to search by IMDB ID
    if (contentId.startsWith("tt")) {
        return 0; // Will need to search by IMDB ID
    }
    
    return 0;
}

bool IdParser::isImdbId(const QString& contentId)
{
    return contentId.startsWith("tt") && contentId.length() >= 9;
}

bool IdParser::isTmdbId(const QString& contentId)
{
    if (contentId.startsWith("tmdb:")) {
        return true;
    }
    bool ok;
    int id = contentId.toInt(&ok);
    return ok && id > 0;
}





