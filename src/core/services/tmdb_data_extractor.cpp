#include "tmdb_data_extractor.h"
#include "configuration.h"
#include <QJsonArray>
#include <QDebug>

QString TmdbDataExtractor::extractMaturityRating(const QJsonObject& tmdbData, const QString& type)
{
    try {
        if (type == "movie") {
            QJsonObject releaseDates = tmdbData["release_dates"].toObject();
            if (!releaseDates.isEmpty()) {
                QJsonArray results = releaseDates["results"].toArray();
                for (const QJsonValue& resultValue : results) {
                    QJsonObject result = resultValue.toObject();
                    if (result["iso_3166_1"].toString() == "US") {
                        QJsonArray releaseDatesList = result["release_dates"].toArray();
                        for (const QJsonValue& releaseValue : releaseDatesList) {
                            QJsonObject release = releaseValue.toObject();
                            QString cert = release["certification"].toString();
                            if (!cert.isEmpty()) {
                                return cert;
                            }
                        }
                    }
                }
            }
        } else if (type == "series" || type == "tv" || type == "show") {
            QJsonObject contentRatings = tmdbData["content_ratings"].toObject();
            if (!contentRatings.isEmpty()) {
                QJsonArray results = contentRatings["results"].toArray();
                for (const QJsonValue& resultValue : results) {
                    QJsonObject result = resultValue.toObject();
                    if (result["iso_3166_1"].toString() == "US") {
                        QString rating = result["rating"].toString();
                        if (!rating.isEmpty()) {
                            return rating;
                        }
                    }
                }
            }
        }
    } catch (...) {
        qDebug() << "Error extracting maturity rating";
    }
    return QString();
}

QString TmdbDataExtractor::getMaturityRatingName(const QString& rating, const QString& type)
{
    if (rating.isEmpty()) {
        return QString();
    }
    
    QString upperRating = rating.toUpper();
    
    if (type == "movie") {
        if (upperRating == "G") return "General Audiences";
        if (upperRating == "PG") return "Parental Guidance Suggested";
        if (upperRating == "PG-13") return "Parents Strongly Cautioned";
        if (upperRating == "R") return "Restricted";
        if (upperRating == "NC-17") return "Adults Only";
    } else if (type == "series" || type == "tv" || type == "show") {
        if (upperRating == "TV-Y") return "All Children";
        if (upperRating == "TV-Y7") return "Directed to Older Children";
        if (upperRating == "TV-G") return "General Audience";
        if (upperRating == "TV-PG") return "Parental Guidance Suggested";
        if (upperRating == "TV-14") return "Parents Strongly Cautioned";
        if (upperRating == "TV-MA") return "Mature Audience Only";
    }
    
    return QString();
}

QJsonObject TmdbDataExtractor::extractCastAndCrew(const QJsonObject& tmdbData)
{
    QJsonObject result;
    QJsonArray castArray;
    QJsonArray crewArray;
    
    try {
        QJsonObject credits = tmdbData["credits"].toObject();
        QJsonArray cast = credits["cast"].toArray();
        QJsonArray crew = credits["crew"].toArray();
        
        for (const QJsonValue& value : cast) {
            castArray.append(value);
        }
        
        for (const QJsonValue& value : crew) {
            crewArray.append(value);
        }
    } catch (...) {
        qDebug() << "Error extracting cast and crew";
    }
    
    result["cast"] = castArray;
    result["crew"] = crewArray;
    return result;
}

QJsonObject TmdbDataExtractor::extractProductionInfo(const QJsonObject& tmdbData, const QString& type)
{
    QJsonObject result;
    QJsonArray companiesArray;
    QJsonArray countriesArray;
    QJsonArray languagesArray;
    
    try {
        QJsonArray productionCompanies = tmdbData["production_companies"].toArray();
        for (const QJsonValue& value : productionCompanies) {
            QJsonObject company = value.toObject();
            QString name = company["name"].toString();
            if (!name.isEmpty()) {
                companiesArray.append(name);
            }
        }
        
        QJsonArray productionCountries = tmdbData["production_countries"].toArray();
        for (const QJsonValue& value : productionCountries) {
            QJsonObject country = value.toObject();
            QString name = country["name"].toString();
            if (!name.isEmpty()) {
                countriesArray.append(name);
            }
        }
        
        QJsonArray spokenLanguages = tmdbData["spoken_languages"].toArray();
        for (const QJsonValue& value : spokenLanguages) {
            QJsonObject lang = value.toObject();
            QString name = lang["name"].toString();
            if (!name.isEmpty()) {
                languagesArray.append(name);
            }
        }
        
        QString originalLanguage = tmdbData["original_language"].toString();
        QString originalTitle;
        if (type == "movie") {
            originalTitle = tmdbData["original_title"].toString();
        } else {
            originalTitle = tmdbData["original_name"].toString();
        }
        
        result["productionCompanies"] = companiesArray;
        result["productionCountries"] = countriesArray;
        result["spokenLanguages"] = languagesArray;
        result["originalLanguage"] = originalLanguage;
        result["originalTitle"] = originalTitle;
    } catch (...) {
        qDebug() << "Error extracting production info";
    }
    
    return result;
}

QJsonObject TmdbDataExtractor::extractReleaseInfo(const QJsonObject& tmdbData, const QString& type)
{
    QJsonObject result;
    
    try {
        if (type == "movie") {
            QString releaseDate = tmdbData["release_date"].toString();
            QString status = tmdbData["status"].toString();
            QString releaseYear;
            if (!releaseDate.isEmpty()) {
                QStringList parts = releaseDate.split("-");
                if (!parts.isEmpty()) {
                    releaseYear = parts[0];
                }
            }
            result["releaseDate"] = releaseDate;
            result["releaseYear"] = releaseYear;
            result["status"] = status;
        } else if (type == "series") {
            QString firstAirDate = tmdbData["first_air_date"].toString();
            QString lastAirDate = tmdbData["last_air_date"].toString();
            QString status = tmdbData["status"].toString();
            QString releaseDate;
            QString releaseYear;
            
            if (!firstAirDate.isEmpty()) {
                if (!lastAirDate.isEmpty() && lastAirDate != firstAirDate) {
                    releaseDate = firstAirDate + " - " + lastAirDate;
                } else {
                    releaseDate = firstAirDate;
                }
                QStringList parts = firstAirDate.split("-");
                if (!parts.isEmpty()) {
                    releaseYear = parts[0];
                }
            }
            
            result["releaseDate"] = releaseDate;
            result["releaseYear"] = releaseYear;
            result["status"] = status;
        }
    } catch (...) {
        qDebug() << "Error extracting release info";
    }
    
    return result;
}

QJsonObject TmdbDataExtractor::extractAdditionalMetadata(const QJsonObject& tmdbData, const QString& type)
{
    QJsonObject result;
    
    try {
        result["budget"] = tmdbData["budget"].toInt();
        result["revenue"] = tmdbData["revenue"].toInt();
        result["tagline"] = tmdbData["tagline"].toString();
        result["voteAverage"] = tmdbData["vote_average"].toDouble();
        result["voteCount"] = tmdbData["vote_count"].toInt();
        result["popularity"] = tmdbData["popularity"].toDouble();
        
        if (type == "movie") {
            result["runtime"] = tmdbData["runtime"].toInt();
        } else {
            QJsonArray episodeRunTime = tmdbData["episode_run_time"].toArray();
            if (!episodeRunTime.isEmpty()) {
                result["runtime"] = episodeRunTime[0].toInt();
            }
            result["numberOfSeasons"] = tmdbData["number_of_seasons"].toInt();
            result["numberOfEpisodes"] = tmdbData["number_of_episodes"].toInt();
        }
    } catch (...) {
        qDebug() << "Error extracting additional metadata";
    }
    
    return result;
}

QString TmdbDataExtractor::extractPosterUrl(const QJsonObject& tmdbData, const QString& imageBaseUrl)
{
    QString baseUrl = imageBaseUrl.isEmpty() ? Configuration::instance().tmdbImageBaseUrl() : imageBaseUrl;
    QString posterPath = tmdbData["poster_path"].toString();
    if (posterPath.isEmpty()) {
        return QString();
    }
    if (posterPath.startsWith("http")) {
        return posterPath;
    }
    return baseUrl + "w500" + posterPath;
}

QString TmdbDataExtractor::extractBackdropUrl(const QJsonObject& tmdbData, const QString& imageBaseUrl)
{
    QString baseUrl = imageBaseUrl.isEmpty() ? Configuration::instance().tmdbImageBaseUrl() : imageBaseUrl;
    QString backdropPath = tmdbData["backdrop_path"].toString();
    if (backdropPath.isEmpty()) {
        return QString();
    }
    if (backdropPath.startsWith("http")) {
        return backdropPath;
    }
    return baseUrl + "w1280" + backdropPath;
}

QString TmdbDataExtractor::extractLogoUrl(const QJsonObject& tmdbData, const QString& imageBaseUrl)
{
    QString baseUrl = imageBaseUrl.isEmpty() ? Configuration::instance().tmdbImageBaseUrl() : imageBaseUrl;
    
    try {
        QJsonObject images = tmdbData["images"].toObject();
        QJsonArray logos = images["logos"].toArray();
        
        if (logos.isEmpty()) {
            return QString();
        }
        
        // Smart logo selection: prioritize English logos
        QJsonObject preferredLogo;
        QJsonObject englishLogo;
        QJsonObject usEnglishLogo;
        
        for (const QJsonValue& logoValue : logos) {
            QJsonObject logo = logoValue.toObject();
            QString country = logo["iso_3166_1"].toString();
            QString language = logo["iso_639_1"].toString();
            
            // Priority 1: US + English (best match)
            if (country == "US" && language == "en") {
                usEnglishLogo = logo;
                break; // Found best match, stop searching
            }
            // Priority 2: Any English logo
            else if (language == "en" && englishLogo.isEmpty()) {
                englishLogo = logo;
            }
            // Fallback: First available logo
            if (preferredLogo.isEmpty()) {
                preferredLogo = logo;
            }
        }
        
        // Select in priority order: US/English > English > First available
        QJsonObject selectedLogo = usEnglishLogo.isEmpty() ? (englishLogo.isEmpty() ? preferredLogo : englishLogo) : usEnglishLogo;
        
        QString logoPath = selectedLogo["file_path"].toString();
        if (logoPath.isEmpty()) {
            return QString();
        }
        if (logoPath.startsWith("http")) {
            return logoPath;
        }
        return baseUrl + "w500" + logoPath;
    } catch (...) {
        qDebug() << "Error extracting logo URL";
        return QString();
    }
}






