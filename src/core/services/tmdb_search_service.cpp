#include "tmdb_search_service.h"
#include "error_service.h"
#include "configuration.h"
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include <algorithm>

TmdbSearchService::TmdbSearchService(QObject* parent)
    : QObject(parent)
    , m_networkManager(std::make_unique<QNetworkAccessManager>(this))
    , m_currentReply(nullptr)
{
}

QUrl TmdbSearchService::buildUrl(const QString& path, const QUrlQuery& query)
{
    Configuration& config = Configuration::instance();
    QUrl url(config.tmdbBaseUrl() + path);
    
    QUrlQuery finalQuery(query);
    finalQuery.addQueryItem("api_key", config.tmdbApiKey());
    url.setQuery(finalQuery);
    
    return url;
}

void TmdbSearchService::searchMovies(const QString& query, int page)
{
    qWarning() << "[TmdbSearchService] ===== searchMovies CALLED =====";
    qWarning() << "[TmdbSearchService] Query:" << query << "Page:" << page;
    
    if (query.trimmed().isEmpty()) {
        qWarning() << "[TmdbSearchService] Empty query, emitting error";
        ErrorService::report("Search query cannot be empty", "MISSING_PARAMS", "TmdbSearchService");
        emit error("Search query cannot be empty");
        return;
    }
    
    if (page < 1) {
        qWarning() << "[TmdbSearchService] Invalid page, emitting error";
        emit error("Page number must be >= 1");
        return;
    }
    
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("query", query);
    urlQuery.addQueryItem("page", QString::number(page));
    
    QUrl url = buildUrl("/search/movie", urlQuery);
    qWarning() << "[TmdbSearchService] Built URL:" << url.toString();
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    
    qWarning() << "[TmdbSearchService] Sending GET request for movies";
    m_currentReply = m_networkManager->get(request);
    m_currentReply->setProperty("requestType", "movies");
    connect(m_currentReply, &QNetworkReply::finished, this, &TmdbSearchService::onMoviesReplyFinished);
    connect(m_currentReply, &QNetworkReply::errorOccurred, this, [this]() {
        qWarning() << "[TmdbSearchService] Network error occurred for movies search";
        handleError(m_currentReply, "Search movies");
    });
    qWarning() << "[TmdbSearchService] Request sent, waiting for reply";
}

void TmdbSearchService::searchTv(const QString& query, int page)
{
    qWarning() << "[TmdbSearchService] ===== searchTv CALLED =====";
    qWarning() << "[TmdbSearchService] Query:" << query << "Page:" << page;
    
    if (query.trimmed().isEmpty()) {
        qWarning() << "[TmdbSearchService] Empty query, emitting error";
        ErrorService::report("Search query cannot be empty", "MISSING_PARAMS", "TmdbSearchService");
        emit error("Search query cannot be empty");
        return;
    }
    
    if (page < 1) {
        qWarning() << "[TmdbSearchService] Invalid page, emitting error";
        emit error("Page number must be >= 1");
        return;
    }
    
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("query", query);
    urlQuery.addQueryItem("page", QString::number(page));
    
    QUrl url = buildUrl("/search/tv", urlQuery);
    qWarning() << "[TmdbSearchService] Built URL:" << url.toString();
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    
    qWarning() << "[TmdbSearchService] Sending GET request for TV";
    m_currentReply = m_networkManager->get(request);
    m_currentReply->setProperty("requestType", "tv");
    connect(m_currentReply, &QNetworkReply::finished, this, &TmdbSearchService::onTvReplyFinished);
    connect(m_currentReply, &QNetworkReply::errorOccurred, this, [this]() {
        qWarning() << "[TmdbSearchService] Network error occurred for TV search";
        handleError(m_currentReply, "Search TV");
    });
    qWarning() << "[TmdbSearchService] Request sent, waiting for reply";
}

void TmdbSearchService::onMoviesReplyFinished()
{
    qWarning() << "[TmdbSearchService] ===== onMoviesReplyFinished CALLED =====";
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        qWarning() << "[TmdbSearchService] ERROR: No reply object";
        return;
    }
    
    qWarning() << "[TmdbSearchService] Reply error code:" << reply->error();
    qWarning() << "[TmdbSearchService] Reply URL:" << reply->url().toString();
    
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "[TmdbSearchService] Network error:" << reply->errorString();
        handleError(reply, "Search movies");
        return;
    }
    
    QByteArray data = reply->readAll();
    qWarning() << "[TmdbSearchService] Received" << data.size() << "bytes of data";
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "[TmdbSearchService] ERROR: Failed to parse JSON";
        emit error("Failed to parse search results");
        reply->deleteLater();
        return;
    }
    
    QJsonObject rootObj = doc.object();
    QJsonArray resultsArray = rootObj["results"].toArray();
    qWarning() << "[TmdbSearchService] Found" << resultsArray.size() << "movies in JSON response";
    
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
    
    qWarning() << "[TmdbSearchService] Emitting moviesFound signal with" << variantList.size() << "movies";
    if (variantList.size() > 0) {
        qWarning() << "[TmdbSearchService] First movie:" << variantList.first().toMap();
    }
    emit moviesFound(variantList);
    qWarning() << "[TmdbSearchService] moviesFound signal emitted";
    reply->deleteLater();
}

void TmdbSearchService::onTvReplyFinished()
{
    qWarning() << "[TmdbSearchService] ===== onTvReplyFinished CALLED =====";
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        qWarning() << "[TmdbSearchService] ERROR: No reply object";
        return;
    }
    
    qWarning() << "[TmdbSearchService] Reply error code:" << reply->error();
    qWarning() << "[TmdbSearchService] Reply URL:" << reply->url().toString();
    
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "[TmdbSearchService] Network error:" << reply->errorString();
        handleError(reply, "Search TV");
        return;
    }
    
    QByteArray data = reply->readAll();
    qWarning() << "[TmdbSearchService] Received" << data.size() << "bytes of data";
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "[TmdbSearchService] ERROR: Failed to parse JSON";
        emit error("Failed to parse search results");
        reply->deleteLater();
        return;
    }
    
    QJsonObject rootObj = doc.object();
    QJsonArray resultsArray = rootObj["results"].toArray();
    qWarning() << "[TmdbSearchService] Found" << resultsArray.size() << "TV shows in JSON response";
    
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
    
    qWarning() << "[TmdbSearchService] Emitting tvFound signal with" << variantList.size() << "TV shows";
    if (variantList.size() > 0) {
        qWarning() << "[TmdbSearchService] First TV show:" << variantList.first().toMap();
    }
    emit tvFound(variantList);
    qWarning() << "[TmdbSearchService] tvFound signal emitted";
    reply->deleteLater();
}

void TmdbSearchService::handleError(QNetworkReply* reply, const QString& context)
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

bool TmdbSearchService::compareByPopularity(const TmdbSearchResult& a, const TmdbSearchResult& b)
{
    return a.popularity > b.popularity;
}

