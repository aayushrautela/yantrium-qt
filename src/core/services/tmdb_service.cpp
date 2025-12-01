#include "tmdb_service.h"
#include "configuration.h"
#include "tmdb_data_extractor.h"
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

TmdbService::TmdbService(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

QUrl TmdbService::buildUrl(const QString& path, const QUrlQuery& query)
{
    Configuration& config = Configuration::instance();
    QUrl url(config.tmdbBaseUrl() + path);
    
    QUrlQuery finalQuery(query);
    finalQuery.addQueryItem("api_key", config.tmdbApiKey());
    url.setQuery(finalQuery);
    
    return url;
}

QString TmdbService::getImageUrl(const QString& path, const QString& size) const
{
    if (path.isEmpty()) {
        return QString();
    }
    if (path.startsWith("http")) {
        return path; // Already full URL
    }
    Configuration& config = Configuration::instance();
    return config.tmdbImageBaseUrl() + size + path;
}

void TmdbService::getTmdbIdFromImdb(const QString& imdbId)
{
    QUrlQuery query;
    query.addQueryItem("external_source", "imdb_id");
    
    QUrl url = buildUrl(QString("/find/%1").arg(imdbId), query);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    
    QNetworkReply* reply = m_networkManager->get(request);
    reply->setProperty("imdbId", imdbId);
    connect(reply, &QNetworkReply::finished, this, &TmdbService::onFindReplyFinished);
    connect(reply, &QNetworkReply::errorOccurred, this, [this, reply]() {
        handleError(reply, "Get TMDB ID from IMDB");
    });
}

void TmdbService::getMovieMetadata(int tmdbId)
{
    QString cacheKey = QString("movie_%1").arg(tmdbId);
    
    // Check cache first
    if (m_cache.contains(cacheKey)) {
        CachedMetadata cached = m_cache[cacheKey];
        if (!cached.isExpired()) {
            // Check if cached data has external_ids
            QJsonObject data = cached.data;
            QJsonObject externalIds = data["external_ids"].toObject();
            if (externalIds.contains("imdb_id")) {
                qDebug() << "[CACHE] Using cached movie metadata for TMDB ID:" << tmdbId;
                emit movieMetadataFetched(tmdbId, data);
                return;
            } else {
                qDebug() << "[CACHE] Cached movie metadata missing external_ids, forcing refresh";
                m_cache.remove(cacheKey);
            }
        } else {
            m_cache.remove(cacheKey);
        }
    }
    
    QUrlQuery query;
    query.addQueryItem("append_to_response", "videos,credits,images,release_dates,external_ids");
    
    QUrl url = buildUrl(QString("/movie/%1").arg(tmdbId), query);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    
    QNetworkReply* reply = m_networkManager->get(request);
    reply->setProperty("tmdbId", tmdbId);
    reply->setProperty("type", "movie");
    connect(reply, &QNetworkReply::finished, this, &TmdbService::onMovieMetadataReplyFinished);
    connect(reply, &QNetworkReply::errorOccurred, this, [this, reply]() {
        handleError(reply, "Get movie metadata");
    });
}

void TmdbService::getTvMetadata(int tmdbId)
{
    QString cacheKey = QString("tv_%1").arg(tmdbId);
    
    // Check cache first
    if (m_cache.contains(cacheKey)) {
        CachedMetadata cached = m_cache[cacheKey];
        if (!cached.isExpired()) {
            // Check if cached data has external_ids
            QJsonObject data = cached.data;
            QJsonObject externalIds = data["external_ids"].toObject();
            if (externalIds.contains("imdb_id")) {
                qDebug() << "[CACHE] Using cached TV metadata for TMDB ID:" << tmdbId;
                emit tvMetadataFetched(tmdbId, data);
                return;
            } else {
                qDebug() << "[CACHE] Cached TV metadata missing external_ids, forcing refresh";
                m_cache.remove(cacheKey);
            }
        } else {
            m_cache.remove(cacheKey);
        }
    }
    
    QUrlQuery query;
    query.addQueryItem("append_to_response", "videos,credits,images,content_ratings,external_ids");
    
    QUrl url = buildUrl(QString("/tv/%1").arg(tmdbId), query);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    
    QNetworkReply* reply = m_networkManager->get(request);
    reply->setProperty("tmdbId", tmdbId);
    reply->setProperty("type", "tv");
    connect(reply, &QNetworkReply::finished, this, &TmdbService::onTvMetadataReplyFinished);
    connect(reply, &QNetworkReply::errorOccurred, this, [this, reply]() {
        handleError(reply, "Get TV metadata");
    });
}

void TmdbService::getCastAndCrew(int tmdbId, const QString& type)
{
    QString endpoint = (type == "movie") ? QString("/movie/%1/credits").arg(tmdbId) 
                                         : QString("/tv/%1/credits").arg(tmdbId);
    
    QUrl url = buildUrl(endpoint);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    
    QNetworkReply* reply = m_networkManager->get(request);
    reply->setProperty("tmdbId", tmdbId);
    reply->setProperty("type", type);
    connect(reply, &QNetworkReply::finished, this, &TmdbService::onCastAndCrewReplyFinished);
    connect(reply, &QNetworkReply::errorOccurred, this, [this, reply]() {
        handleError(reply, "Get cast and crew");
    });
}

void TmdbService::getCatalogItemFromContentId(const QString& contentId, const QString& type)
{
    // Try to extract TMDB ID
    int tmdbId = IdParser::extractTmdbId(contentId);
    
    // If IMDB ID, search for TMDB ID first
    if (tmdbId == 0 && contentId.startsWith("tt")) {
        // Need to get TMDB ID from IMDB first
        // This will emit tmdbIdFound signal, which should trigger metadata fetch
        getTmdbIdFromImdb(contentId);
        return;
    }
    
    if (tmdbId == 0) {
        emit error("Could not extract TMDB ID from content ID");
        return;
    }
    
    // Fetch metadata based on type
    if (type == "movie") {
        getMovieMetadata(tmdbId);
    } else if (type == "series") {
        getTvMetadata(tmdbId);
    } else {
        emit error(QString("Unknown content type: %1").arg(type));
    }
}

void TmdbService::onFindReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    QString imdbId = reply->property("imdbId").toString();
    
    if (reply->error() != QNetworkReply::NoError) {
        handleError(reply, "Get TMDB ID from IMDB");
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (doc.isNull() || !doc.isObject()) {
        emit error("Failed to parse find response");
        reply->deleteLater();
        return;
    }
    
    QJsonObject data = doc.object();
    QJsonArray movieResults = data["movie_results"].toArray();
    QJsonArray tvResults = data["tv_results"].toArray();
    
    int tmdbId = 0;
    if (!movieResults.isEmpty()) {
        tmdbId = movieResults[0].toObject()["id"].toInt();
    } else if (!tvResults.isEmpty()) {
        tmdbId = tvResults[0].toObject()["id"].toInt();
    }
    
    if (tmdbId > 0) {
        emit tmdbIdFound(imdbId, tmdbId);
    } else {
        emit error("TMDB ID not found for IMDB ID: " + imdbId);
    }
    
    reply->deleteLater();
}

void TmdbService::onMovieMetadataReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    int tmdbId = reply->property("tmdbId").toInt();
    
    if (reply->error() != QNetworkReply::NoError) {
        handleError(reply, "Get movie metadata");
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (doc.isNull() || !doc.isObject()) {
        emit error("Failed to parse movie metadata");
        reply->deleteLater();
        return;
    }
    
    QJsonObject data = doc.object();
    
    // Store in cache
    CachedMetadata cached;
    cached.data = data;
    cached.timestamp = QDateTime::currentDateTime();
    m_cache[QString("movie_%1").arg(tmdbId)] = cached;
    
    emit movieMetadataFetched(tmdbId, data);
    reply->deleteLater();
}

void TmdbService::onTvMetadataReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    int tmdbId = reply->property("tmdbId").toInt();
    
    if (reply->error() != QNetworkReply::NoError) {
        handleError(reply, "Get TV metadata");
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (doc.isNull() || !doc.isObject()) {
        emit error("Failed to parse TV metadata");
        reply->deleteLater();
        return;
    }
    
    QJsonObject data = doc.object();
    
    // Store in cache
    CachedMetadata cached;
    cached.data = data;
    cached.timestamp = QDateTime::currentDateTime();
    m_cache[QString("tv_%1").arg(tmdbId)] = cached;
    
    emit tvMetadataFetched(tmdbId, data);
    reply->deleteLater();
}

void TmdbService::onCastAndCrewReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    int tmdbId = reply->property("tmdbId").toInt();
    QString type = reply->property("type").toString();
    
    if (reply->error() != QNetworkReply::NoError) {
        handleError(reply, "Get cast and crew");
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (doc.isNull() || !doc.isObject()) {
        emit error("Failed to parse cast and crew");
        reply->deleteLater();
        return;
    }
    
    QJsonObject data = doc.object();
    emit castAndCrewFetched(tmdbId, type, data);
    reply->deleteLater();
}

void TmdbService::handleError(QNetworkReply* reply, const QString& context)
{
    if (reply->error() == QNetworkReply::OperationCanceledError) {
        qDebug() << context << "Request canceled";
        return;
    }
    
    QString errorMessage = QString("%1 failed: %2").arg(context, reply->errorString());
    qWarning() << errorMessage;
    emit error(errorMessage);
    reply->deleteLater();
}

void TmdbService::clearCache()
{
    m_cache.clear();
}

void TmdbService::clearCacheForId(int tmdbId, const QString& type)
{
    QString cacheKey = QString("%1_%2").arg(type, QString::number(tmdbId));
    m_cache.remove(cacheKey);
}

int TmdbService::getCacheSize() const
{
    return m_cache.size();
}

QJsonObject TmdbService::convertMovieToCatalogItem(const QJsonObject& tmdbData, const QString& contentId)
{
    QJsonObject result;
    
    try {
        // Extract cast and crew
        QJsonObject castAndCrew = TmdbDataExtractor::extractCastAndCrew(tmdbData);
        QJsonArray cast = castAndCrew["cast"].toArray();
        QJsonArray crew = castAndCrew["crew"].toArray();
        
        // Extract directors
        QJsonArray directors;
        for (const QJsonValue& value : crew) {
            QJsonObject person = value.toObject();
            QString job = person["job"].toString().toLower();
            if (job == "director") {
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
        QString logoUrl = TmdbDataExtractor::extractLogoUrl(tmdbData);
        
        // Extract additional metadata
        QJsonObject additionalMetadata = TmdbDataExtractor::extractAdditionalMetadata(tmdbData, "movie");
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
        result["type"] = "movie";
        result["name"] = tmdbData["title"].toString();
        result["poster"] = TmdbDataExtractor::extractPosterUrl(tmdbData);
        result["background"] = TmdbDataExtractor::extractBackdropUrl(tmdbData);
        result["logo"] = logoUrl;
        result["description"] = tmdbData["overview"].toString();
        result["releaseInfo"] = tmdbData["release_date"].toString();
        result["genres"] = genres;
        result["imdbRating"] = QString::number(tmdbData["vote_average"].toDouble());
        result["runtime"] = QString::number(runtime);
        result["director"] = directors;
        result["cast"] = castNames;
        result["castFull"] = cast;
        result["crewFull"] = crew;
        result["videos"] = trailers;
    } catch (...) {
        qDebug() << "Error converting movie to catalog item";
        return QJsonObject();
    }
    
    return result;
}

QJsonObject TmdbService::convertTvToCatalogItem(const QJsonObject& tmdbData, const QString& contentId)
{
    QJsonObject result;
    
    try {
        // Extract cast and crew
        QJsonObject castAndCrew = TmdbDataExtractor::extractCastAndCrew(tmdbData);
        QJsonArray cast = castAndCrew["cast"].toArray();
        QJsonArray crew = castAndCrew["crew"].toArray();
        
        // Extract creators
        QJsonArray creators;
        for (const QJsonValue& value : crew) {
            QJsonObject person = value.toObject();
            QString job = person["job"].toString().toLower();
            if (job == "creator") {
                creators.append(person["name"].toString());
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
        
        // Extract release info
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
        
        // Extract logo
        QString logoUrl = TmdbDataExtractor::extractLogoUrl(tmdbData);
        
        // Extract additional metadata
        QJsonObject additionalMetadata = TmdbDataExtractor::extractAdditionalMetadata(tmdbData, "series");
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
        result["type"] = "series";
        result["name"] = tmdbData["name"].toString();
        result["poster"] = TmdbDataExtractor::extractPosterUrl(tmdbData);
        result["background"] = TmdbDataExtractor::extractBackdropUrl(tmdbData);
        result["logo"] = logoUrl;
        result["description"] = tmdbData["overview"].toString();
        result["releaseInfo"] = releaseInfo;
        result["genres"] = genres;
        result["imdbRating"] = QString::number(tmdbData["vote_average"].toDouble());
        result["runtime"] = QString::number(runtime);
        result["director"] = creators; // Using creators as "director" for series
        result["cast"] = castNames;
        result["castFull"] = cast;
        result["crewFull"] = crew;
        result["videos"] = trailers;
    } catch (...) {
        qDebug() << "Error converting TV to catalog item";
        return QJsonObject();
    }
    
    return result;
}


