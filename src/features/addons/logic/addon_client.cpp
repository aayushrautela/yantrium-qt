#include "addon_client.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QUrlQuery>

AddonClient::AddonClient(const QString& baseUrl, QObject* parent)
    : QObject(parent)
    , m_baseUrl(normalizeBaseUrl(baseUrl))
    , m_networkManager(new QNetworkAccessManager(this))
{
}

AddonClient::~AddonClient()
{
    // QNetworkAccessManager is a child of this object, so it deletes automatically.
    // Individual active replies will delete themselves via deleteLater() in their lambdas.
}

QString AddonClient::normalizeBaseUrl(const QString& url)
{
    QString normalized = url;
    if (normalized.endsWith('/')) {
        normalized.chop(1);
    }
    return normalized;
}

QUrl AddonClient::buildUrl(const QString& path)
{
    QUrl baseUrl(m_baseUrl);
    QString fullPath = path;
    
    // Ensure path starts with '/'
    if (!fullPath.startsWith('/')) {
        fullPath.prepend('/');
    }
    
    // Join paths properly: if base URL has a path, append the new path
    // QUrl.resolved() treats paths starting with '/' as absolute, which replaces the base path
    // Instead, we manually join paths to append
    QString basePath = baseUrl.path();
    
    // Remove trailing slash from base path if present (we'll add it back)
    if (basePath.endsWith('/') && basePath != "/") {
        basePath.chop(1);
    }
    
    // Join paths: basePath + fullPath
    QString joinedPath = basePath + fullPath;
    
    // Set the joined path
    baseUrl.setPath(joinedPath);
    
    return baseUrl;
}

QString AddonClient::extractBaseUrl(const QString& manifestUrl)
{
    QUrl url(manifestUrl);
    
    // According to Stremio spec: base URL = manifest URL with '/manifest.json' removed
    QString path = url.path();
    
    // Remove '/manifest.json' from the path
    if (path.endsWith("/manifest.json")) {
        path = path.left(path.length() - 14); // Remove "/manifest.json"
    } else if (path.endsWith("manifest.json")) {
        path = path.left(path.length() - 13); // Remove "manifest.json"
    }
    
    // Normalize path: ensure it doesn't end with '/' (we'll add it in buildUrl if needed)
    // Actually, let's keep trailing slash for consistency with Stremio behavior
    if (!path.isEmpty() && !path.endsWith('/')) {
        path += '/';
    } else if (path.isEmpty()) {
        path = "/";
    }
    
    // Rebuild URL with cleaned path, removing query and fragment
    QUrl baseUrl = url;
    baseUrl.setPath(path);
    baseUrl.setQuery(QUrlQuery()); // Remove query parameters
    baseUrl.setFragment(QString()); // Remove fragment
    
    // Use default encoding - QUrl will encode what's needed but preserve valid path characters
    return baseUrl.toString(QUrl::RemoveFragment | QUrl::RemoveQuery);
}

void AddonClient::fetchManifest()
{
    QUrl url = buildUrl("/manifest.json");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    
    // Create local pointer (not member variable)
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater(); // Schedule cleanup

        if (reply->error() != QNetworkReply::NoError) {
            emit error(QString("Failed to fetch manifest: %1").arg(reply->errorString()));
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isNull() || !doc.isObject()) {
            emit error("Invalid JSON response for manifest");
            return;
        }

        AddonManifest manifest = AddonManifest::fromJson(doc.object());
        emit manifestFetched(manifest);
    });
}

void AddonClient::getCatalog(const QString& type, const QString& id)
{
    QString path;
    if (id.isEmpty()) {
        path = QString("/catalog/%1.json").arg(type);
    } else {
        path = QString("/catalog/%1/%2.json").arg(type, QUrl::toPercentEncoding(id));
    }
    
    QUrl url = buildUrl(path);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    
    QNetworkReply* reply = m_networkManager->get(request);

    // Capture 'type' to send back with the signal
    connect(reply, &QNetworkReply::finished, this, [this, reply, type]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            // 404 on a catalog usually just means "empty list", not a critical error
            if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 404) {
                emit catalogFetched(type, QJsonArray());
            } else {
                emit error(QString("Failed to fetch catalog: %1").arg(reply->errorString()));
            }
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isNull() || !doc.isObject()) {
            // Fallback to empty if JSON is bad
            emit catalogFetched(type, QJsonArray());
            return;
        }

        QJsonObject obj = doc.object();
        emit catalogFetched(type, obj["metas"].toArray());
    });
}

void AddonClient::getMeta(const QString& type, const QString& id)
{
    QString path = QString("/meta/%1/%2.json").arg(type, QUrl::toPercentEncoding(id));
    QUrl url = buildUrl(path);
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, type, id]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 404) {
                emit error(QString("Metadata not found for %1/%2").arg(type, id));
            } else {
                emit error(QString("Failed to fetch metadata: %1").arg(reply->errorString()));
            }
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isNull() || !doc.isObject()) {
            emit error("Invalid JSON response for metadata");
            return;
        }

        emit metaFetched(type, id, doc.object());
    });
}

void AddonClient::getStreams(const QString& type, const QString& id)
{
    QString path = QString("/stream/%1/%2.json").arg(type, QUrl::toPercentEncoding(id));
    QUrl url = buildUrl(path);
    
    qDebug() << "AddonClient: Requesting streams:" << url.toString();
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, type, id]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 404) {
                emit streamsFetched(type, id, QJsonArray());
            } else {
                emit error(QString("Failed to fetch streams: %1").arg(reply->errorString()));
            }
            return;
        }

        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);

        if (doc.isNull() || !doc.isObject()) {
            qDebug() << "AddonClient: Invalid JSON for streams";
            emit streamsFetched(type, id, QJsonArray());
            return;
        }

        QJsonObject obj = doc.object();
        if (!obj.contains("streams")) {
            emit streamsFetched(type, id, QJsonArray());
            return;
        }

        emit streamsFetched(type, id, obj["streams"].toArray());
    });
}

void AddonClient::search(const QString& type, const QString& catalogId, const QString& query)
{
    // Correct search endpoint format: /catalog/{type}/{id}/search={query}.json
    // The search parameter is part of the path, not a query parameter
    QString encodedQuery = QUrl::toPercentEncoding(query);
    QString path = QString("/catalog/%1/%2/search=%3.json").arg(type, catalogId, encodedQuery);
    QUrl url = buildUrl(path);
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    
    QNetworkReply* reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, type]() {
        reply->deleteLater();
        
        if (reply->error() != QNetworkReply::NoError) {
            if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 404) {
                emit searchResultsFetched(type, QJsonArray());
            } else {
                emit error(QString("Failed to search: %1").arg(reply->errorString()));
            }
            return;
        }
        
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isNull() || !doc.isObject()) {
            emit searchResultsFetched(type, QJsonArray());
            return;
        }
        
        QJsonObject obj = doc.object();
        emit searchResultsFetched(type, obj["metas"].toArray());
    });
}

bool AddonClient::validateManifest(const AddonManifest& manifest)
{
    if (manifest.id().isEmpty()) return false;
    if (manifest.version().isEmpty()) return false;
    if (manifest.name().isEmpty()) return false;
    if (manifest.resources().isEmpty()) return false;
    if (manifest.types().isEmpty()) return false;
    return true;
}