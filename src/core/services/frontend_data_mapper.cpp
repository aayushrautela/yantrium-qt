#include "frontend_data_mapper.h"
#include "configuration.h"
#include <QJsonArray>
#include <QJsonValue>
#include <QDebug>

// Inline helper functions to replace deleted TmdbDataMapper
namespace {
    QString extractPosterUrl(const QJsonObject& tmdbData) {
        QString posterPath = tmdbData["poster_path"].toString();
        if (posterPath.isEmpty()) return QString();
        if (posterPath.startsWith("http")) return posterPath;
        return "https://image.tmdb.org/t/p/w500" + posterPath;
    }
    
    QString extractBackdropUrl(const QJsonObject& tmdbData) {
        QString backdropPath = tmdbData["backdrop_path"].toString();
        if (backdropPath.isEmpty()) return QString();
        if (backdropPath.startsWith("http")) return backdropPath;
        return "https://image.tmdb.org/t/p/w1280" + backdropPath;
    }
    
    QString extractLogoUrl(const QJsonObject& tmdbData) {
        QJsonObject images = tmdbData["images"].toObject();
        QJsonArray logos = images["logos"].toArray();
        if (!logos.isEmpty()) {
            QJsonObject logo = logos[0].toObject();
            QString filePath = logo["file_path"].toString();
            if (!filePath.isEmpty()) {
                if (filePath.startsWith("http")) return filePath;
                return "https://image.tmdb.org/t/p/w500" + filePath;
            }
        }
        return QString();
    }
    
    QJsonObject extractCastAndCrew(const QJsonObject& tmdbData) {
        QJsonObject result;
        result["cast"] = tmdbData["credits"].toObject()["cast"].toArray();
        result["crew"] = tmdbData["credits"].toObject()["crew"].toArray();
        return result;
    }
    
    QJsonObject extractAdditionalMetadata(const QJsonObject& tmdbData, const QString& type) {
        QJsonObject result;
        if (type == "movie") {
            result["runtime"] = tmdbData["runtime"].toInt();
        } else {
            QJsonArray runtimes = tmdbData["episode_run_time"].toArray();
            if (!runtimes.isEmpty()) {
                result["runtime"] = runtimes[0].toInt();
            }
        }
        return result;
    }
    
    QString extractMaturityRating(const QJsonObject& tmdbData, const QString& type) {
        QJsonArray certifications;
        if (type == "movie") {
            QJsonArray results = tmdbData["release_dates"].toObject()["results"].toArray();
            for (const QJsonValue& value : results) {
                QJsonObject country = value.toObject();
                if (country["iso_3166_1"].toString() == "US") {
                    QJsonArray dates = country["release_dates"].toArray();
                    if (!dates.isEmpty()) {
                        QString cert = dates[0].toObject()["certification"].toString();
                        if (!cert.isEmpty()) return cert;
                    }
                }
            }
        } else {
            QJsonArray results = tmdbData["content_ratings"].toObject()["results"].toArray();
            for (const QJsonValue& value : results) {
                QJsonObject country = value.toObject();
                if (country["iso_3166_1"].toString() == "US") {
                    QString rating = country["rating"].toString();
                    if (!rating.isEmpty()) return rating;
                }
            }
        }
        return QString();
    }
}

// Inline helper for TmdbImageUrlBuilder
namespace {
    class ImageUrlBuilder {
    public:
        enum class ImageSize { Small, Medium, Large };
        static QString buildUrl(const QString& path, ImageSize size) {
            if (path.isEmpty()) return QString();
            if (path.startsWith("http")) return path;
            QString sizeStr = (size == ImageSize::Small) ? "w185" : (size == ImageSize::Medium) ? "w500" : "w1280";
            return "https://image.tmdb.org/t/p/" + sizeStr + path;
        }
        static QString buildUrl(const QString& path, const QString& size) {
            if (path.isEmpty()) return QString();
            if (path.startsWith("http")) return path;
            return "https://image.tmdb.org/t/p/" + size + path;
        }
    };
}

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
        QJsonObject castAndCrew = extractCastAndCrew(tmdbData);
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
        QString logoUrl = extractLogoUrl(tmdbData);
        
        // Extract additional metadata
        QJsonObject additionalMetadata = extractAdditionalMetadata(tmdbData, type);
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
        
        result["poster"] = extractPosterUrl(tmdbData);
        result["background"] = extractBackdropUrl(tmdbData);
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
        
        // Extract both IDs from TMDB data
        int tmdbIdInt = tmdbData["id"].toInt();
        if (tmdbIdInt > 0) {
            result["tmdbId"] = QString::number(tmdbIdInt);
        }
        
        QJsonObject externalIds = tmdbData["external_ids"].toObject();
        QString imdbId = externalIds["imdb_id"].toString();
        if (!imdbId.isEmpty()) {
            result["imdbId"] = imdbId;
            result["imdb_id"] = imdbId; // Also set as imdb_id for compatibility
        }
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
        map["backdropUrl"] = extractBackdropUrl(tmdbData);
        map["logoUrl"] = extractLogoUrl(tmdbData);
        map["posterUrl"] = extractPosterUrl(tmdbData);
        
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
        QString maturityRating = extractMaturityRating(tmdbData, type);
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
        QJsonObject castAndCrew = extractCastAndCrew(tmdbData);
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
                personMap["profileImageUrl"] = ImageUrlBuilder::buildUrl(profilePath, ImageUrlBuilder::ImageSize::Small);
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

QVariantMap FrontendDataMapper::mapAddonMetaToDetailVariantMap(const QJsonObject& addonMeta, const QString& contentId, const QString& type)
{
    QVariantMap map;
    
    try {
        // Basic info
        map["id"] = contentId;
        map["type"] = (type == "movie") ? "movie" : "tv";
        
        // Title/Name - Stremio uses "name" field
        QString name = addonMeta["name"].toString();
        map["title"] = name;
        map["name"] = name;
        
        // Description - Stremio uses "description"
        map["description"] = addonMeta["description"].toString();
        map["overview"] = addonMeta["description"].toString();
        
        // Images - Stremio uses "poster", "background", "logo"
        QString poster = addonMeta["poster"].toString();
        QString background = addonMeta["background"].toString();
        QString logo = addonMeta["logo"].toString();
        
        map["posterUrl"] = poster;
        map["backdropUrl"] = background;
        map["logoUrl"] = logo;
        
        // Release dates - Stremio uses "releaseInfo" or "released"
        QString releaseInfo = addonMeta["releaseInfo"].toString();
        QString released = addonMeta["released"].toString();
        
        QString releaseDate;
        if (!releaseInfo.isEmpty()) {
            // releaseInfo can be "2025" or "2023-01-01"
            if (releaseInfo.length() >= 4) {
                releaseDate = releaseInfo.left(4) + "-01-01"; // Use year as date
                if (releaseInfo.length() >= 10) {
                    releaseDate = releaseInfo.left(10); // Full date
                }
            }
        } else if (!released.isEmpty()) {
            // released is ISO format "2025-11-26T00:00:00.000Z"
            releaseDate = released.left(10);
        }
        
        if (!releaseDate.isEmpty()) {
            map["releaseDate"] = formatDateDDMMYYYY(releaseDate);
            map["firstAirDate"] = formatDateDDMMYYYY(releaseDate);
            
            // Extract year
            if (releaseDate.length() >= 4) {
                map["year"] = releaseDate.left(4).toInt();
            }
        }
        
        // Genres - Stremio uses "genres" array
        QVariantList genres;
        if (addonMeta.contains("genres") && addonMeta["genres"].isArray()) {
            QJsonArray genresArray = addonMeta["genres"].toArray();
            for (const QJsonValue& value : genresArray) {
                if (value.isString()) {
                    genres.append(value.toString());
                } else if (value.isObject()) {
                    QJsonObject genreObj = value.toObject();
                    genres.append(genreObj["name"].toString());
                }
            }
        }
        map["genres"] = genres;
        
        // Ratings - Stremio uses "imdbRating" or "rating"
        QString imdbRating = addonMeta["imdbRating"].toString();
        if (imdbRating.isEmpty()) {
            imdbRating = addonMeta["rating"].toString();
        }
        map["imdbRating"] = imdbRating;
        map["tmdbRating"] = imdbRating; // Use same rating if no separate TMDB rating
        
        // IDs - extract from meta or use contentId
        QString imdbId = addonMeta["imdb_id"].toString();
        if (imdbId.isEmpty() && contentId.startsWith("tt")) {
            imdbId = contentId;
        }
        map["imdbId"] = imdbId;
        
        // Extract TMDB ID if available
        QString tmdbId = addonMeta["tmdb_id"].toString();
        if (tmdbId.isEmpty()) {
            tmdbId = addonMeta["tmdbId"].toString();
        }
        map["tmdbId"] = tmdbId;
        
        // Cast - AIOMetadata uses "app_extras.cast" array
        QVariantList castList;
        QJsonArray castArray;
        
        // Check for cast in app_extras first (AIOMetadata format)
        if (addonMeta.contains("app_extras") && addonMeta["app_extras"].isObject()) {
            QJsonObject appExtras = addonMeta["app_extras"].toObject();
            if (appExtras.contains("cast") && appExtras["cast"].isArray()) {
                castArray = appExtras["cast"].toArray();
            }
        }
        // Fallback to direct "cast" field
        if (castArray.isEmpty() && addonMeta.contains("cast") && addonMeta["cast"].isArray()) {
            castArray = addonMeta["cast"].toArray();
        }
        
        for (const QJsonValue& value : castArray) {
            if (value.isString()) {
                // Simple string cast member
                QVariantMap person;
                person["name"] = value.toString();
                castList.append(person);
            } else if (value.isObject()) {
                // Full cast object with name, character, photo, etc.
                QJsonObject personObj = value.toObject();
                QVariantMap person;
                
                // Extract name
                person["name"] = personObj["name"].toString();
                
                // Extract character
                if (personObj.contains("character")) {
                    person["character"] = personObj["character"].toString();
                }
                
                // Extract profile image - AIOMetadata uses "photo" field
                QString photo = personObj["photo"].toString();
                if (photo.isEmpty()) {
                    photo = personObj["profile_path"].toString();
                }
                
                if (!photo.isEmpty() && !photo.startsWith("http")) {
                    person["profileImageUrl"] = "https://image.tmdb.org/t/p/w185" + photo;
                } else if (photo.startsWith("http")) {
                    person["profileImageUrl"] = photo;
                } else {
                    person["profileImageUrl"] = "";
                }
                
                // Copy other fields
                if (personObj.contains("id")) {
                    person["id"] = personObj["id"].toInt();
                }
                if (personObj.contains("order")) {
                    person["order"] = personObj["order"].toInt();
                }
                
                castList.append(person);
            }
        }
        map["castFull"] = castList;
        
        // Crew - AIOMetadata uses "director" and "writer" as strings or arrays
        QVariantList crewList;
        
        // Handle director (can be string or array)
        if (addonMeta.contains("director")) {
            QVariant directorVar = addonMeta["director"];
            if (directorVar.typeId() == QMetaType::QString) {
                QString directorStr = directorVar.toString();
                if (!directorStr.isEmpty()) {
                    // Split by comma if multiple directors
                    QStringList directors = directorStr.split(",");
                    for (const QString& dir : directors) {
                        QVariantMap person;
                        person["name"] = dir.trimmed();
                        person["job"] = "Director";
                        crewList.append(person);
                    }
                }
            } else if (directorVar.typeId() == QMetaType::QJsonArray) {
                QJsonArray directorArray = directorVar.toJsonArray();
                for (const QJsonValue& value : directorArray) {
                    QVariantMap person;
                    person["name"] = value.toString();
                    person["job"] = "Director";
                    crewList.append(person);
                }
            }
        }
        
        // Handle writer (can be string or array)
        if (addonMeta.contains("writer")) {
            QVariant writerVar = addonMeta["writer"];
            if (writerVar.typeId() == QMetaType::QString) {
                QString writerStr = writerVar.toString();
                if (!writerStr.isEmpty()) {
                    // Split by comma if multiple writers
                    QStringList writers = writerStr.split(",");
                    for (const QString& wr : writers) {
                        QVariantMap person;
                        person["name"] = wr.trimmed();
                        person["job"] = "Writer";
                        crewList.append(person);
                    }
                }
            } else if (writerVar.typeId() == QMetaType::QJsonArray) {
                QJsonArray writerArray = writerVar.toJsonArray();
                for (const QJsonValue& value : writerArray) {
                    QVariantMap person;
                    person["name"] = value.toString();
                    person["job"] = "Writer";
                    crewList.append(person);
                }
            }
        }
        
        // Handle standard crew array
        if (addonMeta.contains("crew") && addonMeta["crew"].isArray()) {
            QJsonArray crewArray = addonMeta["crew"].toArray();
            for (const QJsonValue& value : crewArray) {
                if (value.isObject()) {
                    crewList.append(value.toObject().toVariantMap());
                }
            }
        }
        map["crewFull"] = crewList;
        
        // Videos/Trailers - AIOMetadata uses "trailers" or "trailerStreams"
        QVariantList videos;
        
        // Check for trailers array (AIOMetadata format)
        if (addonMeta.contains("trailers") && addonMeta["trailers"].isArray()) {
            QJsonArray trailersArray = addonMeta["trailers"].toArray();
            for (const QJsonValue& value : trailersArray) {
                if (value.isObject()) {
                    QJsonObject trailerObj = value.toObject();
                    QVariantMap trailer;
                    trailer["name"] = trailerObj["name"].toString();
                    // AIOMetadata uses "ytId" or "source" for YouTube ID
                    QString ytId = trailerObj["ytId"].toString();
                    if (ytId.isEmpty()) {
                        ytId = trailerObj["source"].toString();
                    }
                    trailer["key"] = ytId;
                    trailer["site"] = "YouTube";
                    trailer["type"] = trailerObj["type"].toString();
                    videos.append(trailer);
                }
            }
        }
        // Fallback to trailerStreams
        else if (addonMeta.contains("trailerStreams") && addonMeta["trailerStreams"].isArray()) {
            QJsonArray trailerStreamsArray = addonMeta["trailerStreams"].toArray();
            for (const QJsonValue& value : trailerStreamsArray) {
                if (value.isObject()) {
                    QJsonObject trailerObj = value.toObject();
                    QVariantMap trailer;
                    trailer["name"] = trailerObj["title"].toString();
                    trailer["key"] = trailerObj["ytId"].toString();
                    trailer["site"] = "YouTube";
                    videos.append(trailer);
                }
            }
        }
        // Fallback to standard "videos" array
        else if (addonMeta.contains("videos") && addonMeta["videos"].isArray()) {
            QJsonArray videosArray = addonMeta["videos"].toArray();
            for (const QJsonValue& value : videosArray) {
                if (value.isObject()) {
                    QJsonObject video = value.toObject();
                    QVariantMap trailer;
                    trailer["name"] = video["name"].toString();
                    trailer["key"] = video["key"].toString();
                    trailer["site"] = video["site"].toString();
                    videos.append(trailer);
                }
            }
        }
        map["videos"] = videos;
        
        // Runtime - AIOMetadata uses "runtime" as string like "2h22min" or integer
        // Only extract runtime for movies (TV shows have episode runtimes, not a single runtime)
        if (type == "movie" && addonMeta.contains("runtime")) {
            QJsonValue runtimeValue = addonMeta["runtime"];
            int runtime = 0;
            
            if (runtimeValue.isString()) {
                // Parse string format like "2h22min" or "142min"
                QString runtimeStr = runtimeValue.toString();
                // Remove "min" suffix
                runtimeStr = runtimeStr.replace("min", "");
                // Extract hours and minutes
                int hours = 0;
                int minutes = 0;
                if (runtimeStr.contains("h")) {
                    QStringList parts = runtimeStr.split("h");
                    if (parts.size() >= 1) {
                        hours = parts[0].toInt();
                    }
                    if (parts.size() >= 2) {
                        minutes = parts[1].toInt();
                    }
                } else {
                    minutes = runtimeStr.toInt();
                }
                runtime = hours * 60 + minutes;
            } else if (runtimeValue.isDouble()) {
                runtime = runtimeValue.toInt();
            }
            
            if (runtime > 0) {
                map["runtime"] = runtime;
                map["runtimeFormatted"] = formatRuntime(runtime);
            }
        }
        
        // Number of seasons for TV shows
        if (type == "tv" || type == "series") {
            if (addonMeta.contains("numberOfSeasons")) {
                map["numberOfSeasons"] = addonMeta["numberOfSeasons"].toInt();
            }
        }
        
        // Content rating
        // Check app_extras first (AIOMetadata format)
        QString rating;
        if (addonMeta.contains("app_extras") && addonMeta["app_extras"].isObject()) {
            QJsonObject appExtras = addonMeta["app_extras"].toObject();
            if (appExtras.contains("certification")) {
                rating = appExtras["certification"].toString();
            }
        }
        // Fallback to direct fields
        if (rating.isEmpty()) {
            if (addonMeta.contains("certification")) {
                rating = addonMeta["certification"].toString();
            } else if (addonMeta.contains("contentRating")) {
                rating = addonMeta["contentRating"].toString();
            }
        }
        if (!rating.isEmpty()) {
            map["contentRating"] = rating;
        }
        
    } catch (...) {
        qDebug() << "Error converting addon metadata to detail variant map";
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
        // Only use imdb_id as main ID if no ID is set at all
        // Preserve original ID format (e.g., "tmdb:123") as-is for Stremio compatibility
        if (map["id"].toString().isEmpty()) {
            if (!imdbId.isEmpty()) {
                map["id"] = imdbId;
            }
        }
    }
    
    // Extract TMDB ID from separate field if available
    if (item.contains("tmdb_id") || item.contains("tmdbId")) {
        QString tmdbId = item["tmdb_id"].toString();
        if (tmdbId.isEmpty()) {
            tmdbId = item["tmdbId"].toString();
        }
        if (!tmdbId.isEmpty()) {
            map["tmdbId"] = tmdbId;
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
    
    // Extract both IMDB and TMDB IDs from Trakt (needed for identification, but display data comes from TMDB)
    QString imdbId;
    QString tmdbId;
    if (type == "movie") {
        QVariantMap movie = traktItem["movie"].toMap();
        QVariantMap ids = movie["ids"].toMap();
        imdbId = ids["imdb"].toString();
        tmdbId = ids["tmdb"].toString();
    } else if (type == "episode") {
        QVariantMap show = traktItem["show"].toMap();
        QVariantMap showIds = show["ids"].toMap();
        imdbId = showIds["imdb"].toString();
        tmdbId = showIds["tmdb"].toString();
    }
    map["imdbId"] = imdbId;
    map["tmdbId"] = tmdbId;
    // Use IMDB ID as primary identifier (canonical, stable)
    map["id"] = imdbId;
    
    // ALL display data comes from TMDB
    if (!tmdbData.isEmpty()) {
        // Extract TMDB ID from TMDB data if not already set from Trakt
        if (tmdbId.isEmpty()) {
            tmdbId = QString::number(tmdbData["id"].toInt());
            map["tmdbId"] = tmdbId;
        }
        
        if (type == "movie") {
            // Movie display data from TMDB
            map["title"] = tmdbData["title"].toString();
            
            // Extract year from release date
            QString releaseDate = tmdbData["release_date"].toString();
            if (!releaseDate.isEmpty() && releaseDate.length() >= 4) {
                map["year"] = releaseDate.left(4).toInt();
            }
            
            // Extract images from TMDB
            QString posterUrl = extractPosterUrl(tmdbData);
            QString backdropUrl = extractBackdropUrl(tmdbData);
            QString logoUrl = extractLogoUrl(tmdbData);
            
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
            QString posterUrl = extractPosterUrl(tmdbData);
            QString backdropUrl = extractBackdropUrl(tmdbData);
            QString logoUrl = extractLogoUrl(tmdbData);
            
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

QVariantList FrontendDataMapper::mapSearchResultsToVariantList(const QJsonArray& results, [[maybe_unused]] const QString& mediaType)
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
            map["posterUrl"] = ImageUrlBuilder::buildUrl(posterPath, ImageUrlBuilder::ImageSize::Medium);
        }
        
        QString backdropPath = result["backdrop_path"].toString();
        if (!backdropPath.isEmpty()) {
            map["backdropPath"] = backdropPath;
            map["backdropUrl"] = ImageUrlBuilder::buildUrl(backdropPath, "w1280");
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
            map["posterUrl"] = ImageUrlBuilder::buildUrl(posterPath, ImageUrlBuilder::ImageSize::Medium);
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

QVariantMap FrontendDataMapper::enrichItemWithTmdbData(const QVariantMap& item, const QJsonObject& tmdbData, const QString& type)
{
    QVariantMap enrichedItem = item;

    // Add TMDB-specific fields
    enrichedItem["tmdbDataAvailable"] = true;
    
    // Ensure both IDs are preserved/extracted
    // Extract TMDB ID from TMDB data
    int tmdbIdInt = tmdbData["id"].toInt();
    if (tmdbIdInt > 0) {
        enrichedItem["tmdbId"] = QString::number(tmdbIdInt);
    }
    
    // Extract IMDB ID from TMDB external_ids if not already set
    if (enrichedItem["imdbId"].toString().isEmpty()) {
        QJsonObject externalIds = tmdbData["external_ids"].toObject();
        QString imdbId = externalIds["imdb_id"].toString();
        if (!imdbId.isEmpty()) {
            enrichedItem["imdbId"] = imdbId;
        }
    }

    // Extract runtime
    if (type == "movie") {
        int runtime = tmdbData["runtime"].toInt();
        if (runtime > 0) {
            enrichedItem["runtime"] = runtime;
            enrichedItem["runtimeFormatted"] = formatRuntime(runtime);
        }
    } else if (type == "tv" || type == "show") {
        QJsonArray runtimeArray = tmdbData["episode_run_time"].toArray();
        if (!runtimeArray.isEmpty()) {
            int runtime = runtimeArray[0].toInt();
            if (runtime > 0) {
                enrichedItem["runtime"] = runtime;
                enrichedItem["runtimeFormatted"] = formatRuntime(runtime);
            }
        }
    }

    // Extract genres
    QJsonArray genresArray = tmdbData["genres"].toArray();
    QStringList genres;
    for (const QJsonValue& genreValue : genresArray) {
        QJsonObject genre = genreValue.toObject();
        genres.append(genre["name"].toString());
    }
    if (!genres.isEmpty()) {
        enrichedItem["genres"] = genres;
    }

    // Determine badge text based on release date
    QString badgeText = determineBadgeText(tmdbData, type);
    if (!badgeText.isEmpty()) {
        enrichedItem["badgeText"] = badgeText;
    }

    // Update release dates for better badge calculation
    if (type == "movie") {
        enrichedItem["releaseDate"] = tmdbData["release_date"].toString();
    } else if (type == "tv" || type == "show") {
        enrichedItem["firstAirDate"] = tmdbData["first_air_date"].toString();
        enrichedItem["lastAirDate"] = tmdbData["last_air_date"].toString();
        enrichedItem["status"] = tmdbData["status"].toString();
    }

    return enrichedItem;
}

QString FrontendDataMapper::determineBadgeText(const QJsonObject& tmdbData, const QString& type)
{
    QDateTime now = QDateTime::currentDateTime();
    QDateTime releaseDate;

    if (type == "movie") {
        QString releaseDateStr = tmdbData["release_date"].toString();
        if (releaseDateStr.isEmpty()) return QString();

        releaseDate = QDateTime::fromString(releaseDateStr, Qt::ISODate);
    } else if (type == "tv" || type == "show") {
        // For TV shows, use last_air_date for "new season" detection
        QString lastAirDateStr = tmdbData["last_air_date"].toString();
        QString status = tmdbData["status"].toString();

        if (lastAirDateStr.isEmpty()) return QString();

        releaseDate = QDateTime::fromString(lastAirDateStr, Qt::ISODate);

        // Only show "New Season" for shows that are still active
        if (status != "Returning Series") return QString();
    }

    if (!releaseDate.isValid()) return QString();

    // Calculate days since release
    qint64 daysSinceRelease = releaseDate.daysTo(now);

    // Show badge for content released within last 30 days
    if (daysSinceRelease >= 0 && daysSinceRelease <= 30) {
        return type == "movie" ? "Just Released" : "New Season";
    }

    return QString();
}

QString FrontendDataMapper::formatRuntime(int minutes)
{
    if (minutes <= 0) return QString();

    int hours = minutes / 60;
    int mins = minutes % 60;

    if (hours > 0) {
        return QString("%1h %2m").arg(hours).arg(mins);
    } else {
        return QString("%1m").arg(mins);
    }
}

