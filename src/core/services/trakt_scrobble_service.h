#ifndef TRAKT_SCROBBLE_SERVICE_H
#define TRAKT_SCROBBLE_SERVICE_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QVariantList>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "../models/trakt_models.h"
#include "../services/trakt_core_service.h"

class TraktScrobbleService : public QObject
{
    Q_OBJECT

public:
    explicit TraktScrobbleService(QObject* parent = nullptr);
    
    Q_INVOKABLE void scrobbleStart(const QJsonObject& contentData, double progress);
    Q_INVOKABLE void scrobblePause(const QJsonObject& contentData, double progress, bool force = false);
    Q_INVOKABLE void scrobbleStop(const QJsonObject& contentData, double progress);
    Q_INVOKABLE void scrobblePauseImmediate(const QJsonObject& contentData, double progress);
    Q_INVOKABLE void scrobbleStopImmediate(const QJsonObject& contentData, double progress);
    
    // History management
    Q_INVOKABLE void getHistory(const QString& type = QString(), int id = 0,
                               const QDateTime& startAt = QDateTime(), const QDateTime& endAt = QDateTime(),
                               int page = 1, int limit = 100);
    Q_INVOKABLE void getHistoryMovies(const QDateTime& startAt = QDateTime(), const QDateTime& endAt = QDateTime(),
                                     int page = 1, int limit = 100);
    Q_INVOKABLE void getHistoryEpisodes(const QDateTime& startAt = QDateTime(), const QDateTime& endAt = QDateTime(),
                                       int page = 1, int limit = 100);
    Q_INVOKABLE void getHistoryShows(const QDateTime& startAt = QDateTime(), const QDateTime& endAt = QDateTime(),
                                    int page = 1, int limit = 100);
    Q_INVOKABLE void removeFromHistory(const QJsonObject& payload);
    Q_INVOKABLE void removeMovieFromHistory(const QString& imdbId);
    Q_INVOKABLE void removeEpisodeFromHistory(const QString& showImdbId, int season, int episode);
    Q_INVOKABLE void removeShowFromHistory(const QString& imdbId);
    Q_INVOKABLE void removeHistoryByIds(const QVariantList& historyIds);

signals:
    void scrobbleStarted(bool success);
    void scrobblePaused(bool success);
    void scrobbleStopped(bool success);
    void historyFetched(const QVariantList& history);
    void historyRemoved(bool success);
    void error(const QString& message);

private slots:
    void onScrobbleReplyFinished();
    void onHistoryReplyFinished();
    void onHistoryRemoveReplyFinished();

private:
    TraktCoreService& m_coreService;
    QNetworkAccessManager* m_networkManager;
    
    bool validateContentData(const QJsonObject& contentData);
    QJsonObject buildScrobblePayload(const QJsonObject& contentData, double progress);
    QString buildHistoryEndpoint(const QString& type, int id);
    QJsonObject buildHistoryQueryParams(const QDateTime& startAt, const QDateTime& endAt, int page, int limit);
};

#endif // TRAKT_SCROBBLE_SERVICE_H

