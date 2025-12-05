#include "tmdb_data_service.h"
#include <QUrlQuery>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <algorithm>

TmdbDataService::TmdbDataService(QObject* parent)
    : QObject(parent)
    , m_apiClient(new TmdbApiClient(this))
{
    connect(m_apiClient, &TmdbApiClient::error, this, &TmdbDataService::onApiClientError);
    connect(m_apiClient, &TmdbApiClient::cachedResponseReady, this, &TmdbDataService::onCachedResponseReady);
}

void TmdbDataService::onApiClientError(const TmdbErrorInfo& errorInfo)
{
    QString errorMessage;
    if (errorInfo.type == TmdbError::RateLimited) {
        errorMessage = "Rate limited. Please try again later.";
    } else if (errorInfo.type == TmdbError::Unauthorized) {
        errorMessage = "Unauthorized - check TMDB API key";
    } else if (errorInfo.type == TmdbError::NotFound) {
        errorMessage = "Resource not found";
    } else {
        errorMessage = errorInfo.message.isEmpty() ? "TMDB API error" : errorInfo.message;
    }
    emit error(errorMessage);
}

void TmdbDataService::onCachedResponseReady(const QString& path, const QUrlQuery& query, const QJsonObject& data)
{
    qDebug() << "[TmdbDataService] Processing cached response for:" << path;
    
    // Route cached response to appropriate handler based on path pattern
    if (path.startsWith("/find/")) {
        // Extract IMDB ID from path
        QString imdbId = path.mid(6); // Remove "/find/" prefix
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
    } else if (path.startsWith("/movie/") && path.contains("/similar")) {
        // Similar movies
        int tmdbId = path.mid(8, path.indexOf("/similar") - 8).toInt();
        QJsonArray results = data["results"].toArray();
        emit similarMoviesFetched(tmdbId, results);
    } else if (path.startsWith("/tv/") && path.contains("/similar")) {
        // Similar TV shows
        int tmdbId = path.mid(4, path.indexOf("/similar") - 4).toInt();
        QJsonArray results = data["results"].toArray();
        emit similarTvFetched(tmdbId, results);
    } else if (path.startsWith("/movie/") && path.contains("/credits")) {
        // Cast and crew for movie
        int tmdbId = path.mid(8, path.indexOf("/credits") - 8).toInt();
        emit castAndCrewFetched(tmdbId, "movie", data);
    } else if (path.startsWith("/tv/") && path.contains("/credits")) {
        // Cast and crew for TV
        int tmdbId = path.mid(4, path.indexOf("/credits") - 4).toInt();
        emit castAndCrewFetched(tmdbId, "tv", data);
    } else if (path.startsWith("/movie/")) {
        // Movie metadata
        int tmdbId = path.mid(8).toInt();
        emit movieMetadataFetched(tmdbId, data);
    } else if (path.startsWith("/tv/")) {
        // TV metadata
        int tmdbId = path.mid(4).toInt();
        emit tvMetadataFetched(tmdbId, data);
    } else if (path == "/search/movie") {
        // Movie search
        QJsonArray resultsArray = data["results"].toArray();
        QList<TmdbSearchResult> results;
        
        for (const QJsonValue& value : resultsArray) {
            results.append(TmdbSearchResult::fromJson(value.toObject()));
        }
        
        std::sort(results.begin(), results.end(), compareByPopularity);
        
        QVariantList variantList;
        for (const TmdbSearchResult& result : results) {
            QVariantMap map;
            map["id"] = result.id;
            map["title"] = result.title;
            map["name"] = result.name;
            map["overview"] = result.overview;
            map["releaseDate"] = result.releaseDate;
            map["firstAirDate"] = result.firstAirDate;
            map["posterPath"] = result.posterPath;
            map["backdropPath"] = result.backdropPath;
            map["voteAverage"] = result.voteAverage;
            map["voteCount"] = result.voteCount;
            map["popularity"] = result.popularity;
            map["adult"] = result.adult;
            map["mediaType"] = result.mediaType;
            variantList.append(map);
        }
        
        emit moviesFound(variantList);
    } else if (path == "/search/tv") {
        // TV search
        QJsonArray resultsArray = data["results"].toArray();
        QList<TmdbSearchResult> results;
        
        for (const QJsonValue& value : resultsArray) {
            results.append(TmdbSearchResult::fromJson(value.toObject()));
        }
        
        std::sort(results.begin(), results.end(), compareByPopularity);
        
        QVariantList variantList;
        for (const TmdbSearchResult& result : results) {
            QVariantMap map;
            map["id"] = result.id;
            map["title"] = result.title;
            map["name"] = result.name;
            map["overview"] = result.overview;
            map["releaseDate"] = result.releaseDate;
            map["firstAirDate"] = result.firstAirDate;
            map["posterPath"] = result.posterPath;
            map["backdropPath"] = result.backdropPath;
            map["voteAverage"] = result.voteAverage;
            map["voteCount"] = result.voteCount;
            map["popularity"] = result.popularity;
            map["adult"] = result.adult;
            map["mediaType"] = result.mediaType;
            variantList.append(map);
        }
        
        emit tvFound(variantList);
    } else {
        qWarning() << "[TmdbDataService] Unknown cached response path:" << path;
    }
}

void TmdbDataService::getTmdbIdFromImdb(const QString& imdbId)
{
    QUrlQuery query;
    query.addQueryItem("external_source", "imdb_id");
    
    QNetworkReply* reply = m_apiClient->get(QString("/find/%1").arg(imdbId), query);
    if (reply) {
        reply->setProperty("imdbId", imdbId);
        m_replyContexts[reply] = "Get TMDB ID from IMDB";
        connect(reply, &QNetworkReply::finished, this, &TmdbDataService::onFindReplyFinished);
    }
}

void TmdbDataService::getMovieMetadata(int tmdbId)
{
    QUrlQuery query;
    query.addQueryItem("append_to_response", "videos,credits,images,release_dates,external_ids");
    
    QNetworkReply* reply = m_apiClient->get(QString("/movie/%1").arg(tmdbId), query);
    if (reply) {
        reply->setProperty("tmdbId", tmdbId);
        reply->setProperty("type", "movie");
        m_replyContexts[reply] = "Get movie metadata";
        connect(reply, &QNetworkReply::finished, this, &TmdbDataService::onMovieMetadataReplyFinished);
    }
}

void TmdbDataService::getTvMetadata(int tmdbId)
{
    QUrlQuery query;
    query.addQueryItem("append_to_response", "videos,credits,images,content_ratings,external_ids");
    
    QNetworkReply* reply = m_apiClient->get(QString("/tv/%1").arg(tmdbId), query);
    if (reply) {
        reply->setProperty("tmdbId", tmdbId);
        reply->setProperty("type", "tv");
        m_replyContexts[reply] = "Get TV metadata";
        connect(reply, &QNetworkReply::finished, this, &TmdbDataService::onTvMetadataReplyFinished);
    }
}

void TmdbDataService::getCastAndCrew(int tmdbId, const QString& type)
{
    QString path = (type == "movie") ? QString("/movie/%1/credits").arg(tmdbId)
                                     : QString("/tv/%1/credits").arg(tmdbId);
    
    QNetworkReply* reply = m_apiClient->get(path);
    if (reply) {
        reply->setProperty("tmdbId", tmdbId);
        reply->setProperty("type", type);
        m_replyContexts[reply] = "Get cast and crew";
        connect(reply, &QNetworkReply::finished, this, &TmdbDataService::onCastAndCrewReplyFinished);
    }
}

void TmdbDataService::getSimilarMovies(int tmdbId)
{
    QNetworkReply* reply = m_apiClient->get(QString("/movie/%1/similar").arg(tmdbId));
    if (reply) {
        reply->setProperty("tmdbId", tmdbId);
        m_replyContexts[reply] = "Get similar movies";
        connect(reply, &QNetworkReply::finished, this, &TmdbDataService::onSimilarMoviesReplyFinished);
    }
}

void TmdbDataService::getSimilarTv(int tmdbId)
{
    QNetworkReply* reply = m_apiClient->get(QString("/tv/%1/similar").arg(tmdbId));
    if (reply) {
        reply->setProperty("tmdbId", tmdbId);
        m_replyContexts[reply] = "Get similar TV shows";
        connect(reply, &QNetworkReply::finished, this, &TmdbDataService::onSimilarTvReplyFinished);
    }
}

void TmdbDataService::getTvSeasonDetails(int tmdbId, int seasonNumber)
{
    QNetworkReply* reply = m_apiClient->get(QString("/tv/%1/season/%2").arg(tmdbId).arg(seasonNumber));
    if (reply) {
        reply->setProperty("tmdbId", tmdbId);
        reply->setProperty("seasonNumber", seasonNumber);
        m_replyContexts[reply] = "Get TV season details";
        connect(reply, &QNetworkReply::finished, this, &TmdbDataService::onTvSeasonDetailsReplyFinished);
    }
}

void TmdbDataService::searchMovies(const QString& query, int page)
{
    if (query.trimmed().isEmpty()) {
        emit error("Search query cannot be empty");
        return;
    }
    
    if (page < 1) {
        emit error("Page number must be >= 1");
        return;
    }
    
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("query", query);
    urlQuery.addQueryItem("page", QString::number(page));
    
    QNetworkReply* reply = m_apiClient->get("/search/movie", urlQuery);
    if (reply) {
        reply->setProperty("requestType", "movies");
        m_replyContexts[reply] = "Search movies";
        connect(reply, &QNetworkReply::finished, this, &TmdbDataService::onMoviesSearchReplyFinished);
    }
}

void TmdbDataService::searchTv(const QString& query, int page)
{
    if (query.trimmed().isEmpty()) {
        emit error("Search query cannot be empty");
        return;
    }
    
    if (page < 1) {
        emit error("Page number must be >= 1");
        return;
    }
    
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("query", query);
    urlQuery.addQueryItem("page", QString::number(page));
    
    QNetworkReply* reply = m_apiClient->get("/search/tv", urlQuery);
    if (reply) {
        reply->setProperty("requestType", "tv");
        m_replyContexts[reply] = "Search TV";
        connect(reply, &QNetworkReply::finished, this, &TmdbDataService::onTvSearchReplyFinished);
    }
}

void TmdbDataService::onFindReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    QString context = m_replyContexts.take(reply);
    QString imdbId = reply->property("imdbId").toString();
    
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "[TmdbDataService] Find request error for IMDB ID" << imdbId << ":" << reply->errorString();
        emit error(QString("%1 failed: %2").arg(context, reply->errorString()));
        reply->deleteLater();
        return;
    }
    
    // Check HTTP status code
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode != 200) {
        qWarning() << "[TmdbDataService] Find request returned HTTP" << statusCode << "for IMDB ID" << imdbId;
        emit error(QString("Find request failed with HTTP status %1").arg(statusCode));
        reply->deleteLater();
        return;
    }
    
    QByteArray responseData = reply->readAll();
    if (responseData.isEmpty()) {
        qWarning() << "[TmdbDataService] Empty response for find request, IMDB ID:" << imdbId;
        emit error("Empty response from TMDB find endpoint");
        reply->deleteLater();
        return;
    }
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "[TmdbDataService] JSON parse error for find response:" << parseError.errorString();
        qWarning() << "[TmdbDataService] Response data:" << responseData.left(200);
        emit error(QString("Failed to parse find response: %1").arg(parseError.errorString()));
        reply->deleteLater();
        return;
    }
    
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "[TmdbDataService] Find response is not a JSON object, IMDB ID:" << imdbId;
        qWarning() << "[TmdbDataService] Response data:" << responseData.left(200);
        emit error("Failed to parse find response: not a valid JSON object");
        reply->deleteLater();
        return;
    }
    
    QJsonObject data = doc.object();
    
    // Check for TMDB API error messages
    if (data.contains("status_code") && data.contains("status_message")) {
        int apiStatusCode = data["status_code"].toInt();
        QString apiStatusMessage = data["status_message"].toString();
        qWarning() << "[TmdbDataService] TMDB API error in find response:" << apiStatusCode << apiStatusMessage;
        emit error(QString("TMDB API error: %1").arg(apiStatusMessage));
        reply->deleteLater();
        return;
    }
    
    QJsonArray movieResults = data["movie_results"].toArray();
    QJsonArray tvResults = data["tv_results"].toArray();
    
    qDebug() << "[TmdbDataService] Find response for IMDB ID" << imdbId << "- movies:" << movieResults.size() << "TV:" << tvResults.size();
    
    int tmdbId = 0;
    if (!movieResults.isEmpty()) {
        tmdbId = movieResults[0].toObject()["id"].toInt();
        qDebug() << "[TmdbDataService] Found TMDB movie ID:" << tmdbId << "for IMDB ID:" << imdbId;
    } else if (!tvResults.isEmpty()) {
        tmdbId = tvResults[0].toObject()["id"].toInt();
        qDebug() << "[TmdbDataService] Found TMDB TV ID:" << tmdbId << "for IMDB ID:" << imdbId;
    }
    
    if (tmdbId > 0) {
        emit tmdbIdFound(imdbId, tmdbId);
    } else {
        qWarning() << "[TmdbDataService] No TMDB ID found for IMDB ID:" << imdbId;
        emit error("TMDB ID not found for IMDB ID: " + imdbId);
    }
    
    reply->deleteLater();
}

void TmdbDataService::onMovieMetadataReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    QString context = m_replyContexts.take(reply);
    int tmdbId = reply->property("tmdbId").toInt();
    
    if (reply->error() != QNetworkReply::NoError) {
        emit error(QString("%1 failed: %2").arg(context, reply->errorString()));
        reply->deleteLater();
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (doc.isNull() || !doc.isObject()) {
        emit error("Failed to parse movie metadata");
        reply->deleteLater();
        return;
    }
    
    QJsonObject data = doc.object();
    emit movieMetadataFetched(tmdbId, data);
    reply->deleteLater();
}

void TmdbDataService::onTvMetadataReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    QString context = m_replyContexts.take(reply);
    int tmdbId = reply->property("tmdbId").toInt();
    
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "[TmdbDataService] TV metadata request error for TMDB ID" << tmdbId << ":" << reply->errorString();
        emit error(QString("%1 failed: %2").arg(context, reply->errorString()));
        reply->deleteLater();
        return;
    }
    
    // Check HTTP status code
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode != 200) {
        qWarning() << "[TmdbDataService] TV metadata request returned HTTP" << statusCode << "for TMDB ID" << tmdbId;
        emit error(QString("TV metadata request failed with HTTP status %1").arg(statusCode));
        reply->deleteLater();
        return;
    }
    
    QByteArray responseData = reply->readAll();
    if (responseData.isEmpty()) {
        qWarning() << "[TmdbDataService] Empty response for TV metadata, TMDB ID:" << tmdbId;
        emit error("Empty response from TMDB TV metadata endpoint");
        reply->deleteLater();
        return;
    }
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "[TmdbDataService] JSON parse error for TV metadata:" << parseError.errorString();
        emit error(QString("Failed to parse TV metadata: %1").arg(parseError.errorString()));
        reply->deleteLater();
        return;
    }
    
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "[TmdbDataService] TV metadata response is not a JSON object, TMDB ID:" << tmdbId;
        emit error("Failed to parse TV metadata: not a valid JSON object");
        reply->deleteLater();
        return;
    }
    
    QJsonObject data = doc.object();
    
    // Check for TMDB API error messages
    if (data.contains("status_code") && data.contains("status_message")) {
        int apiStatusCode = data["status_code"].toInt();
        QString apiStatusMessage = data["status_message"].toString();
        qWarning() << "[TmdbDataService] TMDB API error in TV metadata:" << apiStatusCode << apiStatusMessage;
        emit error(QString("TMDB API error: %1").arg(apiStatusMessage));
        reply->deleteLater();
        return;
    }
    
    qDebug() << "[TmdbDataService] Successfully fetched TV metadata for TMDB ID:" << tmdbId;
    emit tvMetadataFetched(tmdbId, data);
    reply->deleteLater();
}

void TmdbDataService::onCastAndCrewReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    QString context = m_replyContexts.take(reply);
    int tmdbId = reply->property("tmdbId").toInt();
    QString type = reply->property("type").toString();
    
    if (reply->error() != QNetworkReply::NoError) {
        emit error(QString("%1 failed: %2").arg(context, reply->errorString()));
        reply->deleteLater();
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

void TmdbDataService::onSimilarMoviesReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    QString context = m_replyContexts.take(reply);
    int tmdbId = reply->property("tmdbId").toInt();
    
    if (reply->error() != QNetworkReply::NoError) {
        emit error(QString("%1 failed: %2").arg(context, reply->errorString()));
        reply->deleteLater();
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (doc.isNull() || !doc.isObject()) {
        emit error("Failed to parse similar movies");
        reply->deleteLater();
        return;
    }
    
    QJsonObject data = doc.object();
    QJsonArray results = data["results"].toArray();
    emit similarMoviesFetched(tmdbId, results);
    reply->deleteLater();
}

void TmdbDataService::onSimilarTvReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    QString context = m_replyContexts.take(reply);
    int tmdbId = reply->property("tmdbId").toInt();
    
    if (reply->error() != QNetworkReply::NoError) {
        emit error(QString("%1 failed: %2").arg(context, reply->errorString()));
        reply->deleteLater();
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (doc.isNull() || !doc.isObject()) {
        emit error("Failed to parse similar TV shows");
        reply->deleteLater();
        return;
    }
    
    QJsonObject data = doc.object();
    QJsonArray results = data["results"].toArray();
    emit similarTvFetched(tmdbId, results);
    reply->deleteLater();
}

void TmdbDataService::onMoviesSearchReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    QString context = m_replyContexts.take(reply);
    
    if (reply->error() != QNetworkReply::NoError) {
        emit error(QString("%1 failed: %2").arg(context, reply->errorString()));
        reply->deleteLater();
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (doc.isNull() || !doc.isObject()) {
        emit error("Failed to parse search results");
        reply->deleteLater();
        return;
    }
    
    QJsonArray resultsArray = doc.object()["results"].toArray();
    QList<TmdbSearchResult> results;
    
    for (const QJsonValue& value : resultsArray) {
        results.append(TmdbSearchResult::fromJson(value.toObject()));
    }
    
    // Sort by popularity (highest first)
    std::sort(results.begin(), results.end(), compareByPopularity);
    
    // Convert to QVariantList for QML
    QVariantList variantList;
    for (const TmdbSearchResult& result : results) {
        QVariantMap map;
        map["id"] = result.id;
        map["title"] = result.title;
        map["name"] = result.name;
        map["overview"] = result.overview;
        map["releaseDate"] = result.releaseDate;
        map["firstAirDate"] = result.firstAirDate;
        map["posterPath"] = result.posterPath;
        map["backdropPath"] = result.backdropPath;
        map["voteAverage"] = result.voteAverage;
        map["voteCount"] = result.voteCount;
        map["popularity"] = result.popularity;
        map["adult"] = result.adult;
        map["mediaType"] = result.mediaType;
        variantList.append(map);
    }
    
    qDebug() << "[TmdbDataService] Found" << results.size() << "movies";
    emit moviesFound(variantList);
    reply->deleteLater();
}

void TmdbDataService::onTvSearchReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    QString context = m_replyContexts.take(reply);
    
    if (reply->error() != QNetworkReply::NoError) {
        emit error(QString("%1 failed: %2").arg(context, reply->errorString()));
        reply->deleteLater();
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (doc.isNull() || !doc.isObject()) {
        emit error("Failed to parse search results");
        reply->deleteLater();
        return;
    }
    
    QJsonArray resultsArray = doc.object()["results"].toArray();
    QList<TmdbSearchResult> results;
    
    for (const QJsonValue& value : resultsArray) {
        results.append(TmdbSearchResult::fromJson(value.toObject()));
    }
    
    // Sort by popularity (highest first)
    std::sort(results.begin(), results.end(), compareByPopularity);
    
    // Convert to QVariantList for QML
    QVariantList variantList;
    for (const TmdbSearchResult& result : results) {
        QVariantMap map;
        map["id"] = result.id;
        map["title"] = result.title;
        map["name"] = result.name;
        map["overview"] = result.overview;
        map["releaseDate"] = result.releaseDate;
        map["firstAirDate"] = result.firstAirDate;
        map["posterPath"] = result.posterPath;
        map["backdropPath"] = result.backdropPath;
        map["voteAverage"] = result.voteAverage;
        map["voteCount"] = result.voteCount;
        map["popularity"] = result.popularity;
        map["adult"] = result.adult;
        map["mediaType"] = result.mediaType;
        variantList.append(map);
    }
    
    qDebug() << "[TmdbDataService] Found" << results.size() << "TV shows";
    emit tvFound(variantList);
    reply->deleteLater();
}

void TmdbDataService::onTvSeasonDetailsReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    QString context = m_replyContexts.take(reply);
    int tmdbId = reply->property("tmdbId").toInt();
    int seasonNumber = reply->property("seasonNumber").toInt();
    
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "[TmdbDataService] Season details request error for TMDB ID" << tmdbId << "Season" << seasonNumber << ":" << reply->errorString();
        emit error(QString("%1 failed: %2").arg(context, reply->errorString()));
        reply->deleteLater();
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (doc.isNull() || !doc.isObject()) {
        emit error("Failed to parse season details");
        reply->deleteLater();
        return;
    }
    
    QJsonObject data = doc.object();
    qDebug() << "[TmdbDataService] Season details fetched for TMDB ID" << tmdbId << "Season" << seasonNumber;
    emit tvSeasonDetailsFetched(tmdbId, seasonNumber, data);
    reply->deleteLater();
}

bool TmdbDataService::compareByPopularity(const TmdbSearchResult& a, const TmdbSearchResult& b)
{
    return a.popularity > b.popularity;
}

void TmdbDataService::clearCache()
{
    m_apiClient->clearCache();
}

void TmdbDataService::clearCacheForId(int tmdbId, const QString& type)
{
    QString endpoint = QString("/%1/%2").arg(type, QString::number(tmdbId));
    m_apiClient->clearCacheForEndpoint(endpoint);
}

int TmdbDataService::getCacheSize() const
{
    return m_apiClient->getCacheSize();
}

