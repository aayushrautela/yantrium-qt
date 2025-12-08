#ifndef TRAKT_AUTH_SERVICE_H
#define TRAKT_AUTH_SERVICE_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "../services/trakt_core_service.h"
#include "../services/configuration.h"

struct DeviceCodeResponse
{
    QString deviceCode;
    QString userCode;
    QString verificationUrl;
    int interval;      // Polling interval in seconds
    int expiresIn;     // Expiration time in seconds
};

enum class DeviceCodePollResult
{
    Success,
    Pending,    // Still waiting for user authorization
    Failed,     // Error occurred
    Expired,    // Device code expired
    Denied      // User denied
};

class TraktAuthService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isConfigured READ isConfigured CONSTANT)
    Q_PROPERTY(bool isAuthenticated READ isAuthenticated NOTIFY authenticationStatusChanged)

public:
    bool isConfigured() const;
    bool isAuthenticated() const;
    
    Q_INVOKABLE void checkAuthentication();
    Q_INVOKABLE void generateDeviceCode();
    Q_INVOKABLE void pollForAccessToken(const QString& deviceCode, int intervalSeconds);
    Q_INVOKABLE void refreshToken();
    Q_INVOKABLE void getCurrentUser();
    Q_INVOKABLE void logout();

signals:
    void deviceCodeGenerated(const QString& userCode, const QString& verificationUrl, int expiresIn);
    void authenticationStatusChanged(bool authenticated);
    void userInfoFetched(const QString& username, const QString& slug);
    void error(const QString& message);

private slots:
    void onDeviceCodeReplyFinished();
    void onDeviceTokenReplyFinished();
    void onUserInfoReplyFinished();

public:
    explicit TraktAuthService(QObject* parent = nullptr);
    ~TraktAuthService() = default;
    Q_DISABLE_COPY(TraktAuthService)

private:
    mutable std::shared_ptr<Configuration> m_config;
    TraktCoreService* m_coreService;
    QNetworkAccessManager* m_networkManager;
    QTimer* m_pollTimer;
    QString m_currentDeviceCode;
    int m_currentInterval;
    bool m_isAuthenticated;
    
    void fetchUserInfo(const QString& accessToken);
};

#endif // TRAKT_AUTH_SERVICE_H

