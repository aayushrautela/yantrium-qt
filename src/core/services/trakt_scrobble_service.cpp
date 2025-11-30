#include "trakt_scrobble_service.h"
#include "../database/database_manager.h"
#include "configuration.h"
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include <QtMath>

TraktScrobbleService::TraktScrobbleService(QObject* parent)
    : QObject(parent)
    , m_coreService(TraktCoreService::instance())
    , m_networkManager(new QNetworkAccessManager(this))
{
    DatabaseManager& dbManager = DatabaseManager::instance();
    if (dbManager.isInitialized()) {
        m_coreService.setDatabase(dbManager.database());
        m_coreService.initializeAuth();
    }
}

bool TraktScrobbleService::validateContentData(const QJsonObject& contentData)
{
    QString type = contentData["type"].toString();
    if (type != "movie" && type != "episode") {
        emit error("Invalid content type: " + type);
        return false;
    }
    
    if (contentData["title"].toString().trimmed().isEmpty()) {
        emit error("Missing or empty title");
        return false;
    }
    
    if (contentData["imdbId"].toString().trimmed().isEmpty()) {
        emit error("Missing or empty IMDb ID");
        return false;
    }
    
    if (type == "episode") {
        if (contentData["season"].toInt() < 1) {
            emit error("Invalid season number");
            return false;
        }
        if (contentData["episode"].toInt() < 1) {
            emit error("Invalid episode number");
            return false;
        }
        if (contentData["showTitle"].toString().trimmed().isEmpty()) {
            emit error("Missing or empty show title");
            return false;
        }
        if (contentData["showYear"].toInt() < 1900) {
            emit error("Invalid show year");
            return false;
        }
    }
    
    return true;
}

QJsonObject TraktScrobbleService::buildScrobblePayload(const QJsonObject& contentData, double progress)
{
    QJsonObject payload;
    double clampedProgress = qBound(0.0, progress, 100.0);
    payload["progress"] = qRound(clampedProgress);
    
    QString type = contentData["type"].toString();
    if (type == "movie") {
        QJsonObject movie;
        movie["title"] = contentData["title"].toString();
        if (contentData.contains("year")) {
            movie["year"] = contentData["year"].toInt();
        }
        QJsonObject ids;
        QString imdbId = contentData["imdbId"].toString();
        if (!imdbId.startsWith("tt")) {
            imdbId = "tt" + imdbId;
        }
        ids["imdb"] = imdbId;
        movie["ids"] = ids;
        payload["movie"] = movie;
    } else if (type == "episode") {
        QJsonObject show;
        show["title"] = contentData["showTitle"].toString();
        if (contentData.contains("showYear")) {
            show["year"] = contentData["showYear"].toInt();
        }
        QJsonObject showIds;
        QString showImdbId = contentData["showImdbId"].toString();
        if (showImdbId.isEmpty()) {
            showImdbId = contentData["imdbId"].toString();
        }
        if (!showImdbId.startsWith("tt")) {
            showImdbId = "tt" + showImdbId;
        }
        showIds["imdb"] = showImdbId;
        show["ids"] = showIds;
        payload["show"] = show;
        
        QJsonObject episode;
        episode["season"] = contentData["season"].toInt();
        episode["number"] = contentData["episode"].toInt();
        payload["episode"] = episode;
    }
    
    return payload;
}

void TraktScrobbleService::scrobbleStart(const QJsonObject& contentData, double progress)
{
    if (!validateContentData(contentData)) {
        return;
    }
    
    QJsonObject payload = buildScrobblePayload(contentData, progress);
    m_coreService.apiRequest("/scrobble/start", "POST", payload, this, SLOT(onScrobbleReplyFinished()));
}

void TraktScrobbleService::scrobblePause(const QJsonObject& contentData, double progress, bool force)
{
    if (!validateContentData(contentData)) {
        return;
    }
    
    QJsonObject payload = buildScrobblePayload(contentData, progress);
    m_coreService.apiRequest("/scrobble/pause", "POST", payload, this, SLOT(onScrobbleReplyFinished()));
}

void TraktScrobbleService::scrobbleStop(const QJsonObject& contentData, double progress)
{
    if (!validateContentData(contentData)) {
        return;
    }
    
    int threshold = m_coreService.completionThreshold();
    QString endpoint = (progress >= threshold) ? "/scrobble/stop" : "/scrobble/pause";
    
    QJsonObject payload = buildScrobblePayload(contentData, progress);
    m_coreService.apiRequest(endpoint, "POST", payload, this, SLOT(onScrobbleReplyFinished()));
}

void TraktScrobbleService::scrobblePauseImmediate(const QJsonObject& contentData, double progress)
{
    scrobblePause(contentData, progress, true);
}

void TraktScrobbleService::scrobbleStopImmediate(const QJsonObject& contentData, double progress)
{
    scrobbleStop(contentData, progress);
}

QString TraktScrobbleService::buildHistoryEndpoint(const QString& type, int id)
{
    QString endpoint = "/sync/history";
    if (!type.isEmpty()) {
        endpoint += "/" + type;
        if (id > 0) {
            endpoint += "/" + QString::number(id);
        }
    }
    return endpoint;
}

QJsonObject TraktScrobbleService::buildHistoryQueryParams(const QDateTime& startAt, const QDateTime& endAt, int page, int limit)
{
    QJsonObject params;
    params["page"] = page;
    params["limit"] = limit;
    if (startAt.isValid()) {
        params["start_at"] = startAt.toString(Qt::ISODate);
    }
    if (endAt.isValid()) {
        params["end_at"] = endAt.toString(Qt::ISODate);
    }
    return params;
}

void TraktScrobbleService::getHistory(const QString& type, int id, const QDateTime& startAt,
                                      const QDateTime& endAt, int page, int limit)
{
    QString endpoint = buildHistoryEndpoint(type, id);
    QJsonObject params = buildHistoryQueryParams(startAt, endAt, page, limit);
    m_coreService.apiRequest(endpoint, "GET", params, this, SLOT(onHistoryReplyFinished()));
}

void TraktScrobbleService::getHistoryMovies(const QDateTime& startAt, const QDateTime& endAt, int page, int limit)
{
    getHistory("movies", 0, startAt, endAt, page, limit);
}

void TraktScrobbleService::getHistoryEpisodes(const QDateTime& startAt, const QDateTime& endAt, int page, int limit)
{
    getHistory("episodes", 0, startAt, endAt, page, limit);
}

void TraktScrobbleService::getHistoryShows(const QDateTime& startAt, const QDateTime& endAt, int page, int limit)
{
    getHistory("shows", 0, startAt, endAt, page, limit);
}

void TraktScrobbleService::removeFromHistory(const QJsonObject& payload)
{
    m_coreService.apiRequest("/sync/history/remove", "POST", payload, this, SLOT(onHistoryRemoveReplyFinished()));
}

void TraktScrobbleService::removeMovieFromHistory(const QString& imdbId)
{
    QJsonObject payload;
    QJsonArray movies;
    QJsonObject movie;
    QJsonObject ids;
    QString cleanImdbId = imdbId.startsWith("tt") ? imdbId : "tt" + imdbId;
    ids["imdb"] = cleanImdbId;
    movie["ids"] = ids;
    movies.append(movie);
    payload["movies"] = movies;
    removeFromHistory(payload);
}

void TraktScrobbleService::removeEpisodeFromHistory(const QString& showImdbId, int season, int episode)
{
    QJsonObject payload;
    QJsonArray shows;
    QJsonObject show;
    QJsonObject ids;
    QString cleanImdbId = showImdbId.startsWith("tt") ? showImdbId : "tt" + showImdbId;
    ids["imdb"] = cleanImdbId;
    show["ids"] = ids;
    QJsonArray seasons;
    QJsonObject seasonObj;
    seasonObj["number"] = season;
    QJsonArray episodes;
    QJsonObject episodeObj;
    episodeObj["number"] = episode;
    episodes.append(episodeObj);
    seasonObj["episodes"] = episodes;
    seasons.append(seasonObj);
    show["seasons"] = seasons;
    shows.append(show);
    payload["shows"] = shows;
    removeFromHistory(payload);
}

void TraktScrobbleService::removeShowFromHistory(const QString& imdbId)
{
    QJsonObject payload;
    QJsonArray shows;
    QJsonObject show;
    QJsonObject ids;
    QString cleanImdbId = imdbId.startsWith("tt") ? imdbId : "tt" + imdbId;
    ids["imdb"] = cleanImdbId;
    show["ids"] = ids;
    shows.append(show);
    payload["shows"] = shows;
    removeFromHistory(payload);
}

void TraktScrobbleService::removeHistoryByIds(const QVariantList& historyIds)
{
    QJsonObject payload;
    QJsonArray ids;
    for (const QVariant& id : historyIds) {
        ids.append(id.toInt());
    }
    payload["ids"] = ids;
    removeFromHistory(payload);
}

void TraktScrobbleService::onScrobbleReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    QString endpoint = reply->property("endpoint").toString();
    bool isStart = endpoint.contains("/start");
    bool isStop = endpoint.contains("/stop");
    
    if (reply->error() == QNetworkReply::NoError) {
        if (isStart) {
            emit scrobbleStarted(true);
        } else if (isStop) {
            emit scrobbleStopped(true);
        } else {
            emit scrobblePaused(true);
        }
    } else {
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode == 409) {
            // Already scrobbled - treat as success
            if (isStart) {
                emit scrobbleStarted(true);
            } else if (isStop) {
                emit scrobbleStopped(true);
            } else {
                emit scrobblePaused(true);
            }
        } else {
            emit error("Scrobble failed: " + reply->errorString());
            if (isStart) {
                emit scrobbleStarted(false);
            } else if (isStop) {
                emit scrobbleStopped(false);
            } else {
                emit scrobblePaused(false);
            }
        }
    }
    
    reply->deleteLater();
}

void TraktScrobbleService::onHistoryReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray arr = doc.array();
        QVariantList history;
        for (const QJsonValue& val : arr) {
            history.append(val.toObject().toVariantMap());
        }
        emit historyFetched(history);
    } else {
        emit error("Failed to fetch history: " + reply->errorString());
    }
    
    reply->deleteLater();
}

void TraktScrobbleService::onHistoryRemoveReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    if (reply->error() == QNetworkReply::NoError) {
        emit historyRemoved(true);
    } else {
        emit error("Failed to remove from history: " + reply->errorString());
        emit historyRemoved(false);
    }
    
    reply->deleteLater();
}

