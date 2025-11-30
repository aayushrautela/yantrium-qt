#include "tmdb_search_service.h"
#include "configuration.h"
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include <algorithm>

TmdbSearchService::TmdbSearchService(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
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
    
    QUrl url = buildUrl("/search/movie", urlQuery);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    
    m_currentReply = m_networkManager->get(request);
    m_currentReply->setProperty("requestType", "movies");
    connect(m_currentReply, &QNetworkReply::finished, this, &TmdbSearchService::onMoviesReplyFinished);
    connect(m_currentReply, &QNetworkReply::errorOccurred, this, [this]() {
        handleError(m_currentReply, "Search movies");
    });
}

void TmdbSearchService::searchTv(const QString& query, int page)
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
    
    QUrl url = buildUrl("/search/tv", urlQuery);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    
    m_currentReply = m_networkManager->get(request);
    m_currentReply->setProperty("requestType", "tv");
    connect(m_currentReply, &QNetworkReply::finished, this, &TmdbSearchService::onTvReplyFinished);
    connect(m_currentReply, &QNetworkReply::errorOccurred, this, [this]() {
        handleError(m_currentReply, "Search TV");
    });
}

void TmdbSearchService::onMoviesReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    if (reply->error() != QNetworkReply::NoError) {
        handleError(reply, "Search movies");
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
        map["id"] = result.id();
        map["title"] = result.title();
        map["name"] = result.name();
        map["overview"] = result.overview();
        map["releaseDate"] = result.releaseDate();
        map["firstAirDate"] = result.firstAirDate();
        map["posterPath"] = result.posterPath();
        map["backdropPath"] = result.backdropPath();
        map["voteAverage"] = result.voteAverage();
        map["voteCount"] = result.voteCount();
        map["popularity"] = result.popularity();
        map["adult"] = result.adult();
        map["mediaType"] = result.mediaType();
        variantList.append(map);
    }
    
    qDebug() << "Found" << results.size() << "movies";
    emit moviesFound(variantList);
    reply->deleteLater();
}

void TmdbSearchService::onTvReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    if (reply->error() != QNetworkReply::NoError) {
        handleError(reply, "Search TV");
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
        map["id"] = result.id();
        map["title"] = result.title();
        map["name"] = result.name();
        map["overview"] = result.overview();
        map["releaseDate"] = result.releaseDate();
        map["firstAirDate"] = result.firstAirDate();
        map["posterPath"] = result.posterPath();
        map["backdropPath"] = result.backdropPath();
        map["voteAverage"] = result.voteAverage();
        map["voteCount"] = result.voteCount();
        map["popularity"] = result.popularity();
        map["adult"] = result.adult();
        map["mediaType"] = result.mediaType();
        variantList.append(map);
    }
    
    qDebug() << "Found" << results.size() << "TV shows";
    emit tvFound(variantList);
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
    return a.popularity() > b.popularity();
}

