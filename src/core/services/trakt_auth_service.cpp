#include "trakt_auth_service.h"
#include "../database/database_manager.h"
#include "../database/trakt_auth_dao.h"
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

TraktAuthService& TraktAuthService::instance()
{
    static TraktAuthService instance;
    return instance;
}

TraktAuthService::TraktAuthService(QObject* parent)
    : QObject(parent)
    , m_config(Configuration::instance())
    , m_coreService(TraktCoreService::instance())
    , m_networkManager(new QNetworkAccessManager(this))
    , m_pollTimer(new QTimer(this))
    , m_currentInterval(0)
    , m_isAuthenticated(false)
{
    m_pollTimer->setSingleShot(false);
    connect(m_pollTimer, &QTimer::timeout, this, [this]() {
        if (!m_currentDeviceCode.isEmpty()) {
            pollForAccessToken(m_currentDeviceCode, m_currentInterval);
        }
    });
    
    // Initialize core service database if needed
    DatabaseManager& dbManager = DatabaseManager::instance();
    if (dbManager.isInitialized()) {
        m_coreService.initializeDatabase();
        m_coreService.initializeAuth();
    }
    
    checkAuthentication();
}

bool TraktAuthService::isConfigured() const
{
    return m_config.isTraktConfigured();
}

bool TraktAuthService::isAuthenticated() const
{
    return m_isAuthenticated;
}

void TraktAuthService::checkAuthentication()
{
    m_coreService.checkAuthentication();
    // Connect to authentication status changes (singleton ensures this only happens once)
    // Use a lambda with capture to ensure we're always connected
    static QMetaObject::Connection connection;
    if (!connection) {
        connection = connect(&m_coreService, &TraktCoreService::authenticationStatusChanged, this, [this](bool authenticated) {
            m_isAuthenticated = authenticated;
            emit authenticationStatusChanged(authenticated);
        });
    }
}

void TraktAuthService::generateDeviceCode()
{
    if (!isConfigured()) {
        emit error("Trakt API not configured. Please set TRAKT_CLIENT_ID and TRAKT_CLIENT_SECRET");
        return;
    }
    
    QUrl url(m_config.traktDeviceCodeUrl());
    QJsonObject data;
    data["client_id"] = m_config.traktClientId();
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("trakt-api-version", m_config.traktApiVersion().toUtf8());
    request.setRawHeader("trakt-api-key", m_config.traktClientId().toUtf8());
    
    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(data).toJson());
    connect(reply, &QNetworkReply::finished, this, &TraktAuthService::onDeviceCodeReplyFinished);
    connect(reply, &QNetworkReply::errorOccurred, this, [this, reply]() {
        emit error("Failed to generate device code: " + reply->errorString());
        reply->deleteLater();
    });
}

void TraktAuthService::pollForAccessToken(const QString& deviceCode, int intervalSeconds)
{
    if (!isConfigured()) {
        emit error("Trakt API not configured");
        return;
    }
    
    m_currentDeviceCode = deviceCode;
    m_currentInterval = intervalSeconds;
    
    QUrl url(m_config.traktDeviceTokenUrl());
    QJsonObject data;
    data["code"] = deviceCode;
    data["client_id"] = m_config.traktClientId();
    data["client_secret"] = m_config.traktClientSecret();
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("trakt-api-version", m_config.traktApiVersion().toUtf8());
    request.setRawHeader("trakt-api-key", m_config.traktClientId().toUtf8());
    
    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(data).toJson());
    connect(reply, &QNetworkReply::finished, this, &TraktAuthService::onDeviceTokenReplyFinished);
    connect(reply, &QNetworkReply::errorOccurred, this, [this, reply]() {
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode == 400 || statusCode == 429) {
            // Pending or slow down - continue polling
            if (m_pollTimer->interval() == 0) {
                m_pollTimer->setInterval(m_currentInterval * 1000);
                m_pollTimer->start();
            }
        } else if (statusCode == 404 || statusCode == 409 || statusCode == 410 || statusCode == 418) {
            // Invalid, already used, expired, or denied
            m_pollTimer->stop();
            m_currentDeviceCode.clear();
            if (statusCode == 410) {
                emit error("Device code expired");
            } else if (statusCode == 418) {
                emit error("User denied device authentication");
            } else {
                emit error("Device code invalid or already used");
            }
        } else {
            emit error("Failed to poll for access token: " + reply->errorString());
        }
        reply->deleteLater();
    });
}

void TraktAuthService::refreshToken()
{
    m_coreService.checkAuthentication();
    // Token refresh is handled automatically by TraktCoreService
}

void TraktAuthService::getCurrentUser()
{
    m_coreService.getUserProfile();
    // Connect only if not already connected
    static bool userConnected = false;
    if (!userConnected) {
        connect(&m_coreService, &TraktCoreService::userProfileFetched, this, [this](const QJsonObject& user) {
            QString username = user["username"].toString();
            QJsonObject ids = user["ids"].toObject();
            QString slug = ids["slug"].toString();
            emit userInfoFetched(username, slug);
        });
        userConnected = true;
    }
}

void TraktAuthService::logout()
{
    m_coreService.logout();
    m_isAuthenticated = false;
    m_pollTimer->stop();
    m_currentDeviceCode.clear();
    emit authenticationStatusChanged(false);
}

void TraktAuthService::onDeviceCodeReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    if (reply->error() != QNetworkReply::NoError) {
        emit error("Failed to generate device code: " + reply->errorString());
        reply->deleteLater();
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject data = doc.object();
    
    QString userCode = data["user_code"].toString();
    QString verificationUrl = data["verification_url"].toString();
    int expiresIn = data["expires_in"].toInt();
    int interval = data["interval"].toInt();
    
    m_currentDeviceCode = data["device_code"].toString();
    m_currentInterval = interval;
    
    emit deviceCodeGenerated(userCode, verificationUrl, expiresIn);
    
    // Start polling
    m_pollTimer->setInterval(interval * 1000);
    m_pollTimer->start();
    
    reply->deleteLater();
}

void TraktAuthService::onDeviceTokenReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    
    if (statusCode == 200) {
        // Success
        m_pollTimer->stop();
        m_currentDeviceCode.clear();
        
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject data = doc.object();
        
        QString accessToken = data["access_token"].toString();
        QString refreshToken = data["refresh_token"].toString();
        int expiresIn = data["expires_in"].toInt();
        
        // Save tokens via core service
        DatabaseManager& dbManager = DatabaseManager::instance();
        if (dbManager.isInitialized()) {
            TraktAuthDao dao{};
            TraktAuthRecord record;
            record.accessToken = accessToken;
            record.refreshToken = refreshToken;
            record.expiresIn = expiresIn;
            record.createdAt = QDateTime::currentDateTime();
            record.expiresAt = QDateTime::currentDateTime().addSecs(expiresIn);
            
            dao.upsertTraktAuth(record);
            m_coreService.reloadAuth();
            
            // Fetch user info
            fetchUserInfo(accessToken);
            
            m_isAuthenticated = true;
            emit authenticationStatusChanged(true);
        }
    } else if (statusCode == 400 || statusCode == 429) {
        // Pending or slow down - continue polling
        if (m_pollTimer->interval() == 0) {
            m_pollTimer->setInterval(m_currentInterval * 1000);
            m_pollTimer->start();
        }
    } else if (statusCode == 404 || statusCode == 409 || statusCode == 410 || statusCode == 418) {
        // Invalid, already used, expired, or denied
        m_pollTimer->stop();
        m_currentDeviceCode.clear();
        if (statusCode == 410) {
            emit error("Device code expired");
        } else if (statusCode == 418) {
            emit error("User denied device authentication");
        } else {
            emit error("Device code invalid or already used");
        }
    } else {
        emit error("Failed to poll for access token");
    }
    
    reply->deleteLater();
}

void TraktAuthService::onUserInfoReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject data = doc.object();
    QJsonObject user = data["user"].toObject();
    
    QString username = user["username"].toString();
    QJsonObject ids = user["ids"].toObject();
    QString slug = ids["slug"].toString();
    
    // Update database with username/slug
    DatabaseManager& dbManager = DatabaseManager::instance();
    if (dbManager.isInitialized()) {
        TraktAuthDao dao{};
        auto auth = dao.getTraktAuth();
        if (auth) {
            auth->username = username;
            auth->slug = slug;
            dao.upsertTraktAuth(*auth);
        }
    }
    
    emit userInfoFetched(username, slug);
    reply->deleteLater();
}

void TraktAuthService::fetchUserInfo(const QString& accessToken)
{
    QUrl url(m_config.traktBaseUrl() + "/users/settings");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + accessToken).toUtf8());
    request.setRawHeader("trakt-api-version", m_config.traktApiVersion().toUtf8());
    request.setRawHeader("trakt-api-key", m_config.traktClientId().toUtf8());
    
    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &TraktAuthService::onUserInfoReplyFinished);
    connect(reply, &QNetworkReply::errorOccurred, this, [this, reply]() {
        qWarning() << "[TraktAuthService] Failed to fetch user info:" << reply->errorString();
        reply->deleteLater();
    });
}

