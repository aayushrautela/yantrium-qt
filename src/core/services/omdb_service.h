#ifndef OMDB_SERVICE_H
#define OMDB_SERVICE_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>

class OmdbService : public QObject
{
    Q_OBJECT

public:
    explicit OmdbService(QObject* parent = nullptr);
    
    Q_INVOKABLE void getRatings(const QString& imdbId);

signals:
    void ratingsFetched(const QString& imdbId, const QJsonObject& data);
    void error(const QString& message, const QString& imdbId = QString());

private slots:
    void onRatingsReplyFinished();

private:
    QNetworkAccessManager* m_networkManager;
    
    QUrl buildUrl(const QString& imdbId);
    void handleError(QNetworkReply* reply, const QString& context, const QString& imdbId = QString());
};

#endif // OMDB_SERVICE_H

