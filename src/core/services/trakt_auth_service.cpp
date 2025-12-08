#include "trakt_auth_service.h"
#include "logging_service.h"
#include "logging_service.h"
#include "core/di/service_registry.h"
#include "../database/database_manager.h"
#include "../database/trakt_auth_dao.h"
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>

TraktAuthService::TraktAuthService(QObject* parent)
    : QObject(parent)
    , m_config(ServiceRegistry::instance().resolve<Configuration>())
    , m_coreService(nullptr)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_pollTimer(new QTimer(this))
    , m_currentInterval(0)
    , m_isAuthenticated(false)
{
    // Resolve TraktCoreService from registry (will be set up later in main.cpp)
    // For now, we'll resolve it when needed
    m_pollTimer->setSingleShot(false);
    connect(m_pollTimer, &QTimer::timeout, this, [this]() {
        if (!m_currentDeviceCode.isEmpty()) {
            pollForAccessToken(m_currentDeviceCode, m_currentInterval);
        }
    });
    
    // Don't check authentication in constructor - wait until services are fully registered
    // Authentication will be checked when QML calls checkAuthentication() or when explicitly requested
    // This avoids timing issues with service registration order
}

bool TraktAuthService::isConfigured() const
{
    if (!m_config) {
        m_config = ServiceRegistry::instance().resolve<Configuration>();
    }
    return m_config && m_config->isTraktConfigured();
}

bool TraktAuthService::isAuthenticated() const
{
    return m_isAuthenticated;
}

void TraktAuthService::checkAuthentication()
{
    if (!m_coreService) {
        auto coreService = ServiceRegistry::instance().resolve<TraktCoreService>();
        if (coreService) {
            m_coreService = coreService.get();
        } else {
            LoggingService::logError("TraktAuthService", "TraktCoreService not available in registry");
            return;
        }
    }
    
    // Connect to authentication status changes BEFORE checking (so we receive the signal)
    static QMetaObject::Connection connection;
    if (!connection && m_coreService) {
        connection = connect(m_coreService, &TraktCoreService::authenticationStatusChanged, this, [this](bool authenticated) {
            m_isAuthenticated = authenticated;
            emit authenticationStatusChanged(authenticated);
            LoggingService::logDebug("TraktAuthService", QString("Authentication status changed via signal: %1").arg(authenticated ? "authenticated" : "not authenticated"));
        });
    }
    
    // Ensure core service is initialized
    m_coreService->initializeDatabase();
    m_coreService->initializeAuth();
    
    // Now check authentication (this will emit the signal which we're now connected to)
    // The signal will update m_isAuthenticated via the connection above
    m_coreService->checkAuthentication();
}

void TraktAuthService::generateDeviceCode()
{
    if (!isConfigured()) {
        LoggingService::report("Trakt API not configured. Please set TRAKT_CLIENT_ID and TRAKT_CLIENT_SECRET", "CONFIG_ERROR", "TraktAuthService");
        emit error("Trakt API not configured. Please set TRAKT_CLIENT_ID and TRAKT_CLIENT_SECRET");
        return;
    }
    
    if (!m_config) {
        m_config = ServiceRegistry::instance().resolve<Configuration>();
        if (!m_config) {
            LoggingService::report("Configuration service not available", "SERVICE_ERROR", "TraktAuthService");
            emit error("Configuration service not available");
            return;
        }
    }
    
    QUrl url(m_config->traktDeviceCodeUrl());
    QJsonObject data;
    data["client_id"] = m_config->traktClientId();
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("trakt-api-version", m_config->traktApiVersion().toUtf8());
    request.setRawHeader("trakt-api-key", m_config->traktClientId().toUtf8());
    
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
    
    if (!m_config) {
        m_config = ServiceRegistry::instance().resolve<Configuration>();
        if (!m_config) {
            LoggingService::report("Configuration service not available", "SERVICE_ERROR", "TraktAuthService");
            emit error("Configuration service not available");
            return;
        }
    }
    
    QUrl url(m_config->traktDeviceTokenUrl());
    QJsonObject data;
    data["code"] = deviceCode;
    data["client_id"] = m_config->traktClientId();
    data["client_secret"] = m_config->traktClientSecret();
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("trakt-api-version", m_config->traktApiVersion().toUtf8());
    request.setRawHeader("trakt-api-key", m_config->traktClientId().toUtf8());
    
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
    if (!m_coreService) {
        auto coreService = ServiceRegistry::instance().resolve<TraktCoreService>();
        if (coreService) {
            m_coreService = coreService.get();
        } else {
            LoggingService::logError("TraktAuthService", "TraktCoreService not available in registry");
            return;
        }
    }
    m_coreService->checkAuthentication();
    // Token refresh is handled automatically by TraktCoreService
}

void TraktAuthService::getCurrentUser()
{
    if (!m_coreService) {
        auto coreService = ServiceRegistry::instance().resolve<TraktCoreService>();
        if (coreService) {
            m_coreService = coreService.get();
        } else {
            LoggingService::logError("TraktAuthService", "TraktCoreService not available in registry");
            return;
        }
    }
    m_coreService->getUserProfile();
    // Connect only if not already connected
    static bool userConnected = false;
    if (!userConnected && m_coreService) {
        connect(m_coreService, &TraktCoreService::userProfileFetched, this, [this](const QJsonObject& user) {
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
    if (!m_coreService) {
        auto coreService = ServiceRegistry::instance().resolve<TraktCoreService>();
        if (coreService) {
            m_coreService = coreService.get();
        } else {
            LoggingService::logError("TraktAuthService", "TraktCoreService not available in registry");
            m_isAuthenticated = false;
            m_pollTimer->stop();
            m_currentDeviceCode.clear();
            emit authenticationStatusChanged(false);
            return;
        }
    }
    m_coreService->logout();
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
        auto dbManager = ServiceRegistry::instance().resolve<DatabaseManager>();
        if (dbManager && dbManager->isInitialized()) {
            TraktAuthDao dao{};
            TraktAuthRecord record;
            record.accessToken = accessToken;
            record.refreshToken = refreshToken;
            record.expiresIn = expiresIn;
            record.createdAt = QDateTime::currentDateTime();
            record.expiresAt = QDateTime::currentDateTime().addSecs(expiresIn);
            
            (void)dao.upsertTraktAuth(record);
            if (!m_coreService) {
                auto coreService = ServiceRegistry::instance().resolve<TraktCoreService>();
                if (coreService) {
                    m_coreService = coreService.get();
                }
            }
            if (m_coreService) {
                m_coreService->reloadAuth();
            }
            
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
    auto dbManager = ServiceRegistry::instance().resolve<DatabaseManager>();
    if (dbManager && dbManager->isInitialized()) {
        TraktAuthDao dao{};
        auto auth = dao.getTraktAuth();
        if (auth) {
            auth->username = username;
            auth->slug = slug;
            (void)dao.upsertTraktAuth(*auth);
        }
    }
    
    emit userInfoFetched(username, slug);
    reply->deleteLater();
}

void TraktAuthService::fetchUserInfo(const QString& accessToken)
{
    if (!m_config) {
        m_config = ServiceRegistry::instance().resolve<Configuration>();
        if (!m_config) {
            LoggingService::logError("TraktAuthService", "Configuration service not available");
            return;
        }
    }
    
    QUrl url(m_config->traktBaseUrl() + "/users/settings");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + accessToken).toUtf8());
    request.setRawHeader("trakt-api-version", m_config->traktApiVersion().toUtf8());
    request.setRawHeader("trakt-api-key", m_config->traktClientId().toUtf8());
    
    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &TraktAuthService::onUserInfoReplyFinished);
    connect(reply, &QNetworkReply::errorOccurred, this, [this, reply]() {
        LoggingService::logWarning("TraktAuthService", QString("Failed to fetch user info: %1").arg(reply->errorString()));
        reply->deleteLater();
    });
}

