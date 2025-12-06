#ifndef TMDB_SEARCH_SERVICE_H
#define TMDB_SEARCH_SERVICE_H

#include <QObject>
#include <QString>
#include <QList>
#include <QVariantList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>
#include <memory>
#include "../models/tmdb_models.h"

class TmdbSearchService : public QObject
{
    Q_OBJECT

public:
    explicit TmdbSearchService(QObject* parent = nullptr);
    
    Q_INVOKABLE void searchMovies(const QString& query, int page = 1);
    Q_INVOKABLE void searchTv(const QString& query, int page = 1);

signals:
    void moviesFound(const QVariantList& results);
    void tvFound(const QVariantList& results);
    void error(const QString& message);

private slots:
    void onMoviesReplyFinished();
    void onTvReplyFinished();

private:
    std::unique_ptr<QNetworkAccessManager> m_networkManager;
    QNetworkReply* m_currentReply;
    
    QUrl buildUrl(const QString& path, const QUrlQuery& query = QUrlQuery());
    void handleError(QNetworkReply* reply, const QString& context);
    static bool compareByPopularity(const TmdbSearchResult& a, const TmdbSearchResult& b);
};

#endif // TMDB_SEARCH_SERVICE_H

