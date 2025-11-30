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
    , m_currentReply(nullptr)
{
}

AddonClient::~AddonClient()
{
    if (m_currentReply) {
        m_currentReply->deleteLater();
    }
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
    QString fullPath = path;
    if (!fullPath.startsWith('/')) {
        fullPath.prepend('/');
    }
    return QUrl(m_baseUrl + fullPath);
}

QString AddonClient::extractBaseUrl(const QString& manifestUrl)
{
    QUrl url(manifestUrl);
    QStringList pathSegments = url.path().split('/', Qt::SkipEmptyParts);
    
    // Remove 'manifest.json' if it's the last segment
    if (!pathSegments.isEmpty() && pathSegments.last() == "manifest.json") {
        pathSegments.removeLast();
    }
    
    // Rebuild URL without the manifest.json part
    QUrl baseUrl = url;
    baseUrl.setPath('/' + pathSegments.join('/'));
    return baseUrl.toString(QUrl::RemoveFragment | QUrl::RemoveQuery);
}


void AddonClient::fetchManifest()
{
    QUrl url = buildUrl("/manifest.json");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    
    m_currentReply = m_networkManager->get(request);
    connect(m_currentReply, &QNetworkReply::finished, this, &AddonClient::onManifestReplyFinished);
    connect(m_currentReply, &QNetworkReply::errorOccurred, this, [this](QNetworkReply::NetworkError error) {
        Q_UNUSED(error);
        emit this->error(QString("Network error: %1").arg(m_currentReply->errorString()));
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
    
    m_currentReply = m_networkManager->get(request);
    connect(m_currentReply, &QNetworkReply::finished, this, &AddonClient::onCatalogReplyFinished);
    connect(m_currentReply, &QNetworkReply::errorOccurred, this, [this](QNetworkReply::NetworkError error) {
        Q_UNUSED(error);
        if (m_currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 404) {
            // 404 for catalogs returns empty list
            emit this->catalogFetched("", QJsonArray());
        } else {
            emit this->error(QString("Network error: %1").arg(m_currentReply->errorString()));
        }
    });
}

void AddonClient::getMeta(const QString& type, const QString& id)
{
    QString path = QString("/meta/%1/%2.json").arg(type, QUrl::toPercentEncoding(id));
    QUrl url = buildUrl(path);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    
    m_currentReply = m_networkManager->get(request);
    connect(m_currentReply, &QNetworkReply::finished, this, &AddonClient::onMetaReplyFinished);
    connect(m_currentReply, &QNetworkReply::errorOccurred, this, [this, type, id](QNetworkReply::NetworkError error) {
        Q_UNUSED(error);
        if (m_currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 404) {
            emit this->error(QString("Metadata not found for %1/%2").arg(type, id));
        } else {
            emit this->error(QString("Network error: %1").arg(m_currentReply->errorString()));
        }
    });
}

void AddonClient::getStreams(const QString& type, const QString& id)
{
    QString path = QString("/stream/%1/%2.json").arg(type, QUrl::toPercentEncoding(id));
    QUrl url = buildUrl(path);
    
    qDebug() << "AddonClient: Making GET request to:" << path;
    qDebug() << "AddonClient: Full URL:" << url.toString();
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    
    m_currentReply = m_networkManager->get(request);
    connect(m_currentReply, &QNetworkReply::finished, this, &AddonClient::onStreamsReplyFinished);
    connect(m_currentReply, &QNetworkReply::errorOccurred, this, [this](QNetworkReply::NetworkError error) {
        Q_UNUSED(error);
        if (m_currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 404) {
            // 404 for streams returns empty list
            emit this->streamsFetched("", "", QJsonArray());
        } else {
            emit this->error(QString("Network error: %1").arg(m_currentReply->errorString()));
        }
    });
}

void AddonClient::onManifestReplyFinished()
{
    if (!m_currentReply) return;
    
    if (m_currentReply->error() != QNetworkReply::NoError) {
        emit error(QString("Failed to fetch manifest: %1").arg(m_currentReply->errorString()));
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        return;
    }
    
    QByteArray data = m_currentReply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isNull() || !doc.isObject()) {
        emit error("Invalid JSON response for manifest");
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        return;
    }
    
    AddonManifest manifest = AddonManifest::fromJson(doc.object());
    emit manifestFetched(manifest);
    
    m_currentReply->deleteLater();
    m_currentReply = nullptr;
}

void AddonClient::onCatalogReplyFinished()
{
    if (!m_currentReply) return;
    
    if (m_currentReply->error() != QNetworkReply::NoError) {
        int statusCode = m_currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode == 404) {
            emit catalogFetched("", QJsonArray());
        } else {
            emit error(QString("Failed to fetch catalog: %1").arg(m_currentReply->errorString()));
        }
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        return;
    }
    
    QByteArray data = m_currentReply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isNull() || !doc.isObject()) {
        emit catalogFetched("", QJsonArray());
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        return;
    }
    
    QJsonObject obj = doc.object();
    QJsonArray metas = obj["metas"].toArray();
    
    emit catalogFetched("", metas);
    
    m_currentReply->deleteLater();
    m_currentReply = nullptr;
}

void AddonClient::onMetaReplyFinished()
{
    if (!m_currentReply) return;
    
    if (m_currentReply->error() != QNetworkReply::NoError) {
        int statusCode = m_currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode == 404) {
            emit error("Metadata not found");
        } else {
            emit error(QString("Failed to fetch metadata: %1").arg(m_currentReply->errorString()));
        }
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        return;
    }
    
    QByteArray data = m_currentReply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isNull() || !doc.isObject()) {
        emit error("Invalid JSON response for metadata");
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        return;
    }
    
    // Extract type and id from URL path
    QString path = m_currentReply->url().path();
    QStringList parts = path.split('/', Qt::SkipEmptyParts);
    QString type = parts.size() > 1 ? parts[1] : "";
    QString id = parts.size() > 2 ? QUrl::fromPercentEncoding(parts[2].replace(".json", "").toUtf8()) : "";
    
    emit metaFetched(type, id, doc.object());
    
    m_currentReply->deleteLater();
    m_currentReply = nullptr;
}

void AddonClient::onStreamsReplyFinished()
{
    if (!m_currentReply) return;
    
    if (m_currentReply->error() != QNetworkReply::NoError) {
        int statusCode = m_currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode == 404) {
            emit streamsFetched("", "", QJsonArray());
        } else {
            emit error(QString("Failed to fetch streams: %1").arg(m_currentReply->errorString()));
        }
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        return;
    }
    
    QByteArray data = m_currentReply->readAll();
    
    qDebug() << "AddonClient: Response status:" << m_currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "AddonClient: Raw response data:" << data;
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isNull() || !doc.isObject()) {
        qDebug() << "AddonClient: Invalid JSON response";
        emit streamsFetched("", "", QJsonArray());
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        return;
    }
    
    QJsonObject obj = doc.object();
    
    if (!obj.contains("streams")) {
        qDebug() << "AddonClient: Response missing streams key";
        emit streamsFetched("", "", QJsonArray());
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        return;
    }
    
    QJsonArray streams = obj["streams"].toArray();
    
    qDebug() << "AddonClient: Extracted" << streams.size() << "stream(s) from response";
    
    // Extract type and id from URL path
    QString path = m_currentReply->url().path();
    QStringList parts = path.split('/', Qt::SkipEmptyParts);
    QString type = parts.size() > 1 ? parts[1] : "";
    QString id = parts.size() > 2 ? QUrl::fromPercentEncoding(parts[2].replace(".json", "").toUtf8()) : "";
    
    emit streamsFetched(type, id, streams);
    
    m_currentReply->deleteLater();
    m_currentReply = nullptr;
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

