#include "omdb_service.h"
#include "logging_service.h"
#include "logging_service.h"
#include "configuration.h"
#include "core/di/service_registry.h"
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QJsonDocument>

OmdbService::OmdbService(QObject* parent)
    : QObject(parent)
    , m_networkManager(std::make_unique<QNetworkAccessManager>(this))
{
}

QUrl OmdbService::buildUrl(const QString& imdbId)
{
    auto config = ServiceRegistry::instance().resolve<Configuration>();
    if (!config) {
        LoggingService::logError("OmdbService", "Configuration service not available");
        return QUrl();
    }
    QUrl url("http://www.omdbapi.com/");
    
    QUrlQuery query;
    query.addQueryItem("i", imdbId);
    query.addQueryItem("apikey", config->omdbApiKey());
    url.setQuery(query);
    
    return url;
}

void OmdbService::getRatings(const QString& imdbId)
{
    if (imdbId.isEmpty() || !imdbId.startsWith("tt")) {
        LoggingService::report("Invalid IMDB ID", "INVALID_PARAMS", "OmdbService");
        emit error("Invalid IMDB ID");
        return;
    }
    
    // Check if API key is configured
    auto config = ServiceRegistry::instance().resolve<Configuration>();
    if (!config || config->omdbApiKey().isEmpty()) {
        LoggingService::logDebug("OmdbService", "OMDB API key not set, skipping ratings fetch");
        return; // Silently skip if key is not configured
    }
    
    QUrl url = buildUrl(imdbId);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    
    QNetworkReply* reply = m_networkManager->get(request);
    reply->setProperty("imdbId", imdbId);
    connect(reply, &QNetworkReply::finished, this, &OmdbService::onRatingsReplyFinished);
    connect(reply, &QNetworkReply::errorOccurred, this, [this, reply, imdbId]() {
        handleError(reply, "Get OMDB ratings", imdbId);
    });
}

void OmdbService::onRatingsReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    QString imdbId = reply->property("imdbId").toString();
    
    if (reply->error() != QNetworkReply::NoError) {
        handleError(reply, "Get OMDB ratings", imdbId);
        reply->deleteLater();
        return;
    }
    
    QByteArray data = reply->readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        emit this->error(QString("Failed to parse OMDB response: %1").arg(error.errorString()), imdbId);
        reply->deleteLater();
        return;
    }
    
    QJsonObject json = doc.object();
    
    // Check if response is successful
    if (json["Response"].toString() != "True") {
        QString errorMsg = json["Error"].toString("Unknown error");
        emit this->error(QString("OMDB API error: %1").arg(errorMsg), imdbId);
        reply->deleteLater();
        return;
    }
    
    emit ratingsFetched(imdbId, json);
    reply->deleteLater();
}

void OmdbService::handleError(QNetworkReply* reply, const QString& context, const QString& imdbId)
{
    QString errorString = reply ? reply->errorString() : "Unknown error";
    // Only log errors, don't emit them to avoid breaking the UI when key is missing
    LoggingService::logDebug("OmdbService", QString("%1 error: %2").arg(context, errorString));
    // Don't emit error signal for authentication issues - these are expected when key is missing
    if (reply && reply->error() != QNetworkReply::AuthenticationRequiredError) {
        emit error(QString("%1: %2").arg(context, errorString), imdbId);
    }
    if (reply) {
        reply->deleteLater();
    }
}

