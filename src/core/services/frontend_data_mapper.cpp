#include "frontend_data_mapper.h"
#include "tmdb_data_mapper.h"
#include "configuration.h"
#include <QJsonArray>
#include <QJsonValue>
#include <QDebug>

QString FrontendDataMapper::formatDateDDMMYYYY(const QString& dateString)
{
    if (dateString.isEmpty()) {
        return QString();
    }
    
    // Expected format: YYYY-MM-DD
    QStringList parts = dateString.split("-");
    if (parts.size() >= 3) {
        return QString("%1-%2-%3").arg(parts[2], parts[1], parts[0]);
    }
    
    return dateString; // Return as-is if format is unexpected
}

QJsonObject FrontendDataMapper::mapTmdbToCatalogItem(const QJsonObject& tmdbData, const QString& contentId, const QString& type)
{
    QJsonObject result;
    
    try {
        // Extract cast and crew
        QJsonObject castAndCrew = TmdbDataMapper::extractCastAndCrew(tmdbData);
        QJsonArray cast = castAndCrew["cast"].toArray();
        QJsonArray crew = castAndCrew["crew"].toArray();
        
        // Extract directors/creators
        QJsonArray directors;
        for (const QJsonValue& value : crew) {
            QJsonObject person = value.toObject();
            QString job = person["job"].toString().toLower();
            if ((type == "movie" && job == "director") || ((type == "tv" || type == "series") && job == "creator")) {
                directors.append(person["name"].toString());
            }
        }
        
        // Extract genres
        QJsonArray genres;
        QJsonArray genresArray = tmdbData["genres"].toArray();
        for (const QJsonValue& value : genresArray) {
            QJsonObject genre = value.toObject();
            genres.append(genre["name"].toString());
        }
        
        // Extract trailers
        QJsonArray trailers;
        QJsonObject videos = tmdbData["videos"].toObject();
        QJsonArray videoResults = videos["results"].toArray();
        for (const QJsonValue& value : videoResults) {
            QJsonObject video = value.toObject();
            if (video["type"].toString() == "Trailer" && video["site"].toString() == "YouTube") {
                QJsonObject trailer;
                trailer["name"] = video["name"];
                trailer["key"] = video["key"];
                trailer["site"] = video["site"];
                trailers.append(trailer);
            }
        }
        
        // Extract logo
        QString logoUrl = TmdbDataMapper::extractLogoUrl(tmdbData);
        
        // Extract additional metadata
        QJsonObject additionalMetadata = TmdbDataMapper::extractAdditionalMetadata(tmdbData, type);
        int runtime = additionalMetadata["runtime"].toInt();
        
        // Extract cast names (first 10)
        QJsonArray castNames;
        int count = 0;
        for (const QJsonValue& value : cast) {
            if (count >= 10) break;
            QJsonObject person = value.toObject();
            castNames.append(person["name"].toString());
            count++;
        }
        
        // Build result
        result["id"] = contentId;
        result["type"] = type;
        
        if (type == "movie") {
            result["name"] = tmdbData["title"].toString();
            result["releaseInfo"] = tmdbData["release_date"].toString();
        } else {
            result["name"] = tmdbData["name"].toString();
            QString firstAirDate = tmdbData["first_air_date"].toString();
            QString lastAirDate = tmdbData["last_air_date"].toString();
            QString releaseInfo;
            if (!firstAirDate.isEmpty()) {
                if (!lastAirDate.isEmpty() && lastAirDate != firstAirDate) {
                    releaseInfo = firstAirDate + " - " + lastAirDate;
                } else {
                    releaseInfo = firstAirDate;
                }
            }
            result["releaseInfo"] = releaseInfo;
        }
        
        result["poster"] = TmdbDataMapper::extractPosterUrl(tmdbData);
        result["background"] = TmdbDataMapper::extractBackdropUrl(tmdbData);
        result["logo"] = logoUrl;
        result["description"] = tmdbData["overview"].toString();
        result["genres"] = genres;
        result["imdbRating"] = QString::number(tmdbData["vote_average"].toDouble());
        result["runtime"] = QString::number(runtime);
        result["director"] = directors;
        result["cast"] = castNames;
        result["castFull"] = cast;
        result["crewFull"] = crew;
        result["videos"] = trailers;
    } catch (...) {
        qDebug() << "Error converting TMDB data to catalog item";
        return QJsonObject();
    }
    
    return result;
}

QVariantMap FrontendDataMapper::mapTmdbToDetailVariantMap(const QJsonObject& tmdbData, const QString& contentId, const QString& type)
{
    QVariantMap map;
    
    try {
        // Basic info
        map["id"] = contentId;
        // Use "tv" to match database (UI can still display as "show")
        map["type"] = (type == "movie") ? "movie" : "tv";
        
        // Title/Name - always use "title" for frontend
        if (type == "movie") {
            map["title"] = tmdbData["title"].toString();
            map["name"] = tmdbData["title"].toString();
        } else {
            map["title"] = tmdbData["name"].toString();
            map["name"] = tmdbData["name"].toString();
        }
        
        // Images - always use "posterUrl", "backdropUrl", "logoUrl"
        map["backdropUrl"] = TmdbDataMapper::extractBackdropUrl(tmdbData);
        map["logoUrl"] = TmdbDataMapper::extractLogoUrl(tmdbData);
        map["posterUrl"] = TmdbDataMapper::extractPosterUrl(tmdbData);
        
        // Description - always use "description" for frontend
        map["description"] = tmdbData["overview"].toString();
        map["overview"] = tmdbData["overview"].toString(); // Keep for backward compatibility
        
        // Release dates - format to DD-MM-YYYY
        if (type == "movie") {
            QString releaseDate = tmdbData["release_date"].toString();
            map["releaseDate"] = formatDateDDMMYYYY(releaseDate);
            map["firstAirDate"] = formatDateDDMMYYYY(releaseDate);
            
            // Extract year
            if (!releaseDate.isEmpty() && releaseDate.length() >= 4) {
                map["year"] = releaseDate.left(4).toInt();
            }
        } else {
            QString firstAirDate = tmdbData["first_air_date"].toString();
            map["firstAirDate"] = formatDateDDMMYYYY(firstAirDate);
            map["releaseDate"] = formatDateDDMMYYYY(firstAirDate);
            
            // Extract year
            if (!firstAirDate.isEmpty() && firstAirDate.length() >= 4) {
                map["year"] = firstAirDate.left(4).toInt();
            }
            
            // Number of seasons
            map["numberOfSeasons"] = tmdbData["number_of_seasons"].toInt();
        }
        
        // Content rating
        QString maturityRating = TmdbDataMapper::extractMaturityRating(tmdbData, type);
        map["contentRating"] = maturityRating;
        
        // Genres
        QVariantList genres;
        QJsonArray genresArray = tmdbData["genres"].toArray();
        for (const QJsonValue& value : genresArray) {
            QJsonObject genre = value.toObject();
            genres.append(genre["name"].toString());
        }
        map["genres"] = genres;
        
        // Ratings
        double voteAverage = tmdbData["vote_average"].toDouble();
        map["tmdbRating"] = QString::number(voteAverage, 'f', 1);
        
        // IMDB rating from external_ids (placeholder - will be filled by OMDB)
        map["imdbRating"] = "";
        map["rtRating"] = ""; // Placeholder
        map["mcRating"] = ""; // Placeholder
        map["metascore"] = ""; // Placeholder
        
        // IDs
        QJsonObject externalIds = tmdbData["external_ids"].toObject();
        map["imdbId"] = externalIds["imdb_id"].toString();
        map["tmdbId"] = QString::number(tmdbData["id"].toInt());
        
        // Cast and crew
        QJsonObject castAndCrew = TmdbDataMapper::extractCastAndCrew(tmdbData);
        QJsonArray cast = castAndCrew["cast"].toArray();
        QJsonArray crew = castAndCrew["crew"].toArray();
        
        QVariantList castList;
        QVariantList crewList;
        
        // Build full profile image URLs for cast
        for (const QJsonValue& value : cast) {
            QJsonObject person = value.toObject();
            QVariantMap personMap = person.toVariantMap();
            
            // Build full profile image URL
            QString profilePath = person["profile_path"].toString();
            if (!profilePath.isEmpty() && !profilePath.startsWith("http")) {
                personMap["profileImageUrl"] = TmdbImageUrlBuilder::buildUrl(profilePath, TmdbImageUrlBuilder::ImageSize::Small);
            } else if (profilePath.startsWith("http")) {
                personMap["profileImageUrl"] = profilePath;
            } else {
                personMap["profileImageUrl"] = "";
            }
            
            castList.append(personMap);
        }
        for (const QJsonValue& value : crew) {
            crewList.append(value.toObject().toVariantMap());
        }
        
        map["castFull"] = castList;
        map["crewFull"] = crewList;
        
        // Videos/Trailers
        QVariantList videos;
        QJsonObject videosObj = tmdbData["videos"].toObject();
        QJsonArray videoResults = videosObj["results"].toArray();
        for (const QJsonValue& value : videoResults) {
            QJsonObject video = value.toObject();
            if (video["type"].toString() == "Trailer" && video["site"].toString() == "YouTube") {
                QVariantMap trailer;
                trailer["name"] = video["name"].toString();
                trailer["key"] = video["key"].toString();
                trailer["site"] = video["site"].toString();
                videos.append(trailer);
            }
        }
        map["videos"] = videos;
        
    } catch (...) {
        qDebug() << "Error converting TMDB data to detail variant map";
        return QVariantMap();
    }
    
    return map;
}

QVariantMap FrontendDataMapper::mapCatalogItemToVariantMap(const QJsonObject& item, const QString& baseUrl)
{
    QVariantMap map;
    
    // Helper function to resolve relative URLs
    auto resolveUrl = [&baseUrl](const QString& url) -> QString {
        if (url.isEmpty()) {
            return url;
        }
        // If already absolute, return as-is
        if (url.startsWith("http://") || url.startsWith("https://")) {
            return url;
        }
        // If relative, resolve against base URL
        if (!baseUrl.isEmpty() && url.startsWith("/")) {
            // Absolute path relative to base URL
            QString normalizedBase = baseUrl;
            if (normalizedBase.endsWith('/')) {
                normalizedBase.chop(1);
            }
            return normalizedBase + url;
        } else if (!baseUrl.isEmpty() && !url.startsWith("/")) {
            // Relative path
            QString normalizedBase = baseUrl;
            if (!normalizedBase.endsWith('/')) {
                normalizedBase += '/';
            }
            return normalizedBase + url;
        }
        return url;
    };
    
    // Extract common fields
    map["id"] = item["id"].toString();
    map["type"] = item["type"].toString();
    map["title"] = item["title"].toString();
    map["name"] = item["name"].toString(); // Some items use "name" instead of "title"
    if (map["title"].toString().isEmpty()) {
        map["title"] = map["name"].toString();
    }
    map["description"] = item["description"].toString();
    
    // Resolve poster URL - always use "posterUrl"
    QString poster = item["poster"].toString();
    QString resolvedPoster = resolveUrl(poster);
    map["poster"] = resolvedPoster;
    map["posterUrl"] = resolvedPoster; // Alias for QML
    
    // Resolve background/backdrop URL - always use "backdropUrl"
    QString background = item["background"].toString();
    QString resolvedBackground = resolveUrl(background);
    map["background"] = resolvedBackground;
    map["backdropUrl"] = resolvedBackground; // Alias for QML
    
    // Resolve logo URL - always use "logoUrl"
    QString logo = item["logo"].toString();
    QString resolvedLogo = resolveUrl(logo);
    map["logo"] = resolvedLogo;
    map["logoUrl"] = resolvedLogo; // Alias for QML
    
    // Extract IDs
    QVariant idVar = item["id"];
    if (idVar.typeId() == QMetaType::QJsonObject) {
        QJsonObject ids = item["id"].toObject();
        map["imdbId"] = ids["imdb"].toString();
        map["tmdbId"] = ids["tmdb"].toString();
        map["traktId"] = ids["trakt"].toString();
    } else {
        // ID is a string - could be "tmdb:123" or "tt1234567" format
        QString idStr = item["id"].toString();
        map["id"] = idStr;
        
        // Try to extract TMDB ID from "tmdb:123" format
        if (idStr.startsWith("tmdb:")) {
            QString tmdbId = idStr.mid(5); // Remove "tmdb:" prefix
            map["tmdbId"] = tmdbId;
        }
    }
    
    // Extract IMDB ID from separate field if available
    if (item.contains("imdb_id")) {
        QString imdbId = item["imdb_id"].toString();
        map["imdbId"] = imdbId;
        // If ID is not set or is tmdb format, use imdb_id as the main ID
        if (map["id"].toString().isEmpty() || map["id"].toString().startsWith("tmdb:")) {
            if (!imdbId.isEmpty()) {
                map["id"] = imdbId;
            }
        }
    }
    
    // Extract year - check multiple sources
    bool yearFound = false;
    if (item.contains("year")) {
        QVariant yearVar = item["year"];
        if (yearVar.typeId() == QMetaType::Int || yearVar.typeId() == QMetaType::LongLong) {
            map["year"] = yearVar.toInt();
            yearFound = true;
        } else {
            QString yearStr = yearVar.toString();
            bool ok;
            int year = yearStr.toInt(&ok);
            if (ok && year > 1900 && year < 2100) {
                map["year"] = year;
                yearFound = true;
            }
        }
    }
    
    if (!yearFound && item.contains("releaseInfo")) {
        QString releaseInfo = item["releaseInfo"].toString();
        // releaseInfo can be "2025" (just year) or "2023-01-01" (full date) format
        if (releaseInfo.length() >= 4) {
            bool ok;
            int year = releaseInfo.left(4).toInt(&ok);
            if (ok && year > 1900 && year < 2100) {
                map["year"] = year;
                yearFound = true;
            }
        }
    }
    
    if (!yearFound && item.contains("released")) {
        QString released = item["released"].toString();
        // released is ISO date format "2025-11-26T00:00:00.000Z"
        if (released.length() >= 4) {
            bool ok;
            int year = released.left(4).toInt(&ok);
            if (ok && year > 1900 && year < 2100) {
                map["year"] = year;
            }
        }
    }
    
    // Extract rating - check multiple sources
    if (item.contains("imdbRating")) {
        QString rating = item["imdbRating"].toString();
        map["rating"] = rating;
    } else if (item.contains("rating")) {
        QString rating = item["rating"].toString();
        map["rating"] = rating;
    }
    
    // Extract genres
    if (item.contains("genres")) {
        QJsonArray genres = item["genres"].toArray();
        QStringList genreList;
        for (const QJsonValue& genre : genres) {
            genreList.append(genre.toString());
        }
        map["genres"] = genreList;
    }

    // === DATA NORMALIZATION FOR QML COMPATIBILITY ===
    // Ensure ALL expected fields exist with proper types and defaults

    // Required string fields with fallbacks
    if (map["title"].toString().isEmpty()) {
        map["title"] = map["name"].toString();  // Fallback to name
    }
    if (map["title"].toString().isEmpty()) {
        map["title"] = "Unknown Title";  // Final fallback
    }

    if (map["posterUrl"].toString().isEmpty()) {
        map["posterUrl"] = map["poster"].toString();  // Use poster as posterUrl
    }
    if (map["posterUrl"].toString().isEmpty()) {
        map["posterUrl"] = "";  // Explicit empty string
    }

    // Required numeric fields with validation
    if (!map.contains("year") || map["year"].toInt() <= 0) {
        map["year"] = 0;  // Default to 0
    }

    // Required string fields
    if (!map.contains("rating")) {
        map["rating"] = "";  // Default empty
    }

    if (!map.contains("description")) {
        map["description"] = "";  // Default empty
    }

    if (!map.contains("id")) {
        map["id"] = "";  // Default empty
    }

    // Optional fields for ListModel compatibility
    if (!map.contains("progress")) {
        map["progress"] = 0.0;
    }

    if (!map.contains("progressPercent")) {
        map["progressPercent"] = 0.0;
    }

    if (!map.contains("badgeText")) {
        map["badgeText"] = "";
    }

    return map;
}

QVariantMap FrontendDataMapper::mapContinueWatchingItem(const QVariantMap& traktItem, const QJsonObject& tmdbData)
{
    QVariantMap map;
    
    // Extract type from Trakt
    QString type = traktItem["type"].toString();
    map["type"] = type;
    
    // Extract progress from Trakt (only Trakt provides watch progress)
    double progress = traktItem["progress"].toDouble();
    map["progress"] = progress;
    map["progressPercent"] = progress;
    
    // Extract episode information from Trakt (only Trakt provides episode-level data)
    if (type == "episode") {
        QVariantMap episode = traktItem["episode"].toMap();
        if (!episode.isEmpty()) {
            map["season"] = episode["season"].toInt();
            map["episode"] = episode["number"].toInt();
            map["episodeTitle"] = episode["title"].toString(); // Episode title from Trakt
        } else {
            // Set defaults to avoid missing data
            map["season"] = 0;
            map["episode"] = 0;
            map["episodeTitle"] = "";
        }
    }
    
    // Extract IMDB ID from Trakt (needed for identification, but display data comes from TMDB)
    QString imdbId;
    if (type == "movie") {
        QVariantMap movie = traktItem["movie"].toMap();
        QVariantMap ids = movie["ids"].toMap();
        imdbId = ids["imdb"].toString();
    } else if (type == "episode") {
        QVariantMap show = traktItem["show"].toMap();
        QVariantMap showIds = show["ids"].toMap();
        imdbId = showIds["imdb"].toString();
    }
    map["imdbId"] = imdbId;
    map["id"] = imdbId;
    
    // ALL display data comes from TMDB
    if (!tmdbData.isEmpty()) {
        if (type == "movie") {
            // Movie display data from TMDB
            map["title"] = tmdbData["title"].toString();
            
            // Extract year from release date
            QString releaseDate = tmdbData["release_date"].toString();
            if (!releaseDate.isEmpty() && releaseDate.length() >= 4) {
                map["year"] = releaseDate.left(4).toInt();
            }
            
            // Extract images from TMDB
            QString posterUrl = TmdbDataMapper::extractPosterUrl(tmdbData);
            QString backdropUrl = TmdbDataMapper::extractBackdropUrl(tmdbData);
            QString logoUrl = TmdbDataMapper::extractLogoUrl(tmdbData);
            
            // Fallback: backdrop -> poster
            if (backdropUrl.isEmpty() && !posterUrl.isEmpty()) {
                backdropUrl = posterUrl;
            }
            
            map["posterUrl"] = posterUrl;
            map["backdropUrl"] = backdropUrl;
            map["logoUrl"] = logoUrl;
            map["description"] = tmdbData["overview"].toString();
            
        } else if (type == "episode") {
            // TV Show display data from TMDB (show-level, not episode-level)
            map["title"] = tmdbData["name"].toString();
            
            // Extract year from first air date
            QString firstAirDate = tmdbData["first_air_date"].toString();
            if (!firstAirDate.isEmpty() && firstAirDate.length() >= 4) {
                map["year"] = firstAirDate.left(4).toInt();
            }
            
            // Extract images from TMDB
            QString posterUrl = TmdbDataMapper::extractPosterUrl(tmdbData);
            QString backdropUrl = TmdbDataMapper::extractBackdropUrl(tmdbData);
            QString logoUrl = TmdbDataMapper::extractLogoUrl(tmdbData);
            
            // Fallback: backdrop -> poster
            if (backdropUrl.isEmpty() && !posterUrl.isEmpty()) {
                backdropUrl = posterUrl;
            }
            
            map["posterUrl"] = posterUrl;
            map["backdropUrl"] = backdropUrl;
            map["logoUrl"] = logoUrl;
            map["description"] = tmdbData["overview"].toString();
        }
    }
    
    // Extract watched_at from Trakt (metadata, not display)
    map["watchedAt"] = traktItem["paused_at"].toString();
    
    // === DATA NORMALIZATION FOR QML COMPATIBILITY ===
    if (map["title"].toString().isEmpty()) {
        map["title"] = "Unknown";
    }
    
    if (!map.contains("posterUrl")) {
        map["posterUrl"] = "";
    }
    
    if (!map.contains("backdropUrl")) {
        map["backdropUrl"] = "";
    }
    
    if (!map.contains("logoUrl")) {
        map["logoUrl"] = "";
    }
    
    if (!map.contains("type")) {
        map["type"] = "";
    }
    
    if (!map.contains("season")) {
        map["season"] = 0;
    }
    
    if (!map.contains("episode")) {
        map["episode"] = 0;
    }
    
    if (!map.contains("episodeTitle")) {
        map["episodeTitle"] = "";
    }
    
    if (!map.contains("year") || map["year"].toInt() <= 0) {
        map["year"] = 0;
    }
    
    if (!map.contains("progress")) {
        map["progress"] = 0.0;
    }
    
    if (!map.contains("progressPercent")) {
        map["progressPercent"] = 0.0;
    }
    
    return map;
}

QVariantList FrontendDataMapper::mapSearchResultsToVariantList(const QJsonArray& results, const QString& mediaType)
{
    QVariantList variantList;
    
    for (const QJsonValue& value : results) {
        if (!value.isObject()) continue;
        
        QJsonObject result = value.toObject();
        QVariantMap map;
        
        // Use consistent field names
        map["id"] = QString("tmdb:%1").arg(result["id"].toInt());
        map["tmdbId"] = QString::number(result["id"].toInt());
        map["title"] = result["title"].toString();
        map["name"] = result["name"].toString();
        map["overview"] = result["overview"].toString();
        map["description"] = result["overview"].toString(); // Alias
        map["releaseDate"] = result["release_date"].toString();
        map["firstAirDate"] = result["first_air_date"].toString();
        
        // Build image URLs
        QString posterPath = result["poster_path"].toString();
        if (!posterPath.isEmpty()) {
            map["posterPath"] = posterPath;
            map["posterUrl"] = TmdbImageUrlBuilder::buildUrl(posterPath, TmdbImageUrlBuilder::ImageSize::Medium);
        }
        
        QString backdropPath = result["backdrop_path"].toString();
        if (!backdropPath.isEmpty()) {
            map["backdropPath"] = backdropPath;
            map["backdropUrl"] = TmdbImageUrlBuilder::buildUrl(backdropPath, "w1280");
        }
        
        map["voteAverage"] = result["vote_average"].toDouble();
        map["voteCount"] = result["vote_count"].toInt();
        map["popularity"] = result["popularity"].toDouble();
        map["adult"] = result["adult"].toBool();
        map["mediaType"] = result["media_type"].toString();
        
        variantList.append(map);
    }
    
    return variantList;
}

QVariantList FrontendDataMapper::mapSimilarItemsToVariantList(const QJsonArray& results, const QString& type)
{
    QVariantList items;
    
    for (const QJsonValue& value : results) {
        if (!value.isObject()) continue;
        
        QJsonObject item = value.toObject();
        QVariantMap map;
        
        // Extract basic info
        int tmdbId = item["id"].toInt();
        map["id"] = QString("tmdb:%1").arg(tmdbId);
        map["tmdbId"] = QString::number(tmdbId);  // Store raw TMDB ID for direct use
        map["type"] = type;
        
        if (type == "movie") {
            map["title"] = item["title"].toString();
            map["name"] = item["title"].toString();
            
            // Extract year from release_date
            QString releaseDate = item["release_date"].toString();
            if (!releaseDate.isEmpty() && releaseDate.length() >= 4) {
                map["year"] = releaseDate.left(4).toInt();
            }
        } else {
            map["title"] = item["name"].toString();
            map["name"] = item["name"].toString();
            
            // Extract year from first_air_date
            QString firstAirDate = item["first_air_date"].toString();
            if (!firstAirDate.isEmpty() && firstAirDate.length() >= 4) {
                map["year"] = firstAirDate.left(4).toInt();
            }
        }
        
        // Poster URL
        QString posterPath = item["poster_path"].toString();
        if (!posterPath.isEmpty()) {
            map["posterUrl"] = TmdbImageUrlBuilder::buildUrl(posterPath, TmdbImageUrlBuilder::ImageSize::Medium);
        }
        
        // Rating
        double voteAverage = item["vote_average"].toDouble();
        if (voteAverage > 0) {
            map["rating"] = QString::number(voteAverage, 'f', 1);
        }
        
        items.append(map);
    }
    
    return items;
}

QVariantMap FrontendDataMapper::mergeOmdbRatings(QVariantMap& detailMap, const QJsonObject& omdbData)
{
    // Extract IMDb rating
    QString imdbRating = omdbData["imdbRating"].toString();
    if (!imdbRating.isEmpty() && imdbRating != "N/A") {
        detailMap["imdbRating"] = imdbRating;
    }
    
    // Extract Metascore
    QString metascore = omdbData["Metascore"].toString();
    if (!metascore.isEmpty() && metascore != "N/A") {
        detailMap["metascore"] = metascore;
    }
    
    // Extract Rotten Tomatoes ratings
    QJsonArray ratings = omdbData["Ratings"].toArray();
    for (const QJsonValue& value : ratings) {
        QJsonObject rating = value.toObject();
        QString source = rating["Source"].toString();
        QString ratingValue = rating["Value"].toString();
        
        if (source == "Rotten Tomatoes") {
            detailMap["rtRating"] = ratingValue;
            // Also store in omdbRatings array for compatibility
            if (!detailMap.contains("omdbRatings")) {
                detailMap["omdbRatings"] = QVariantList();
            }
            QVariantList rtRatings = detailMap["omdbRatings"].toList();
            QVariantMap rtRating;
            rtRating["source"] = "Rotten Tomatoes";
            rtRating["value"] = ratingValue;
            rtRatings.append(rtRating);
            detailMap["omdbRatings"] = rtRatings;
        }
    }
    
    return detailMap;
}

