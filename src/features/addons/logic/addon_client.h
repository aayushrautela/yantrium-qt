#ifndef ADDON_CLIENT_H
#define ADDON_CLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <memory>
#include "../models/addon_manifest.h"

class AddonClient : public QObject
{
    Q_OBJECT

public:
    explicit AddonClient(const QString& baseUrl, QObject* parent = nullptr);
    ~AddonClient();
    
    // Fetch manifest from addon server
    void fetchManifest();
    
    // Fetch catalog
    void getCatalog(const QString& type, const QString& id = QString());
    
    // Fetch metadata
    void getMeta(const QString& type, const QString& id);
    
    // Fetch streams
    void getStreams(const QString& type, const QString& id);
    
    // Search for content
    void search(const QString& type, const QString& query);
    
    // Static helper methods
    static QString extractBaseUrl(const QString& manifestUrl);
    static bool validateManifest(const AddonManifest& manifest);
    
signals:
    void manifestFetched(const AddonManifest& manifest);
    void catalogFetched(const QString& type, const QJsonArray& metas);
    void metaFetched(const QString& type, const QString& id, const QJsonObject& meta);
    void streamsFetched(const QString& type, const QString& id, const QJsonArray& streams);
    void searchResultsFetched(const QString& type, const QJsonArray& metas);
    void error(const QString& errorMessage);

private:
    QString normalizeBaseUrl(const QString& url);
    QUrl buildUrl(const QString& path);
    
    QString m_baseUrl;
    QNetworkAccessManager* m_networkManager;
    
    // REMOVED: QNetworkReply* m_currentReply; 
    // REMOVED: private slots for replies
};

#endif // ADDON_CLIENT_H