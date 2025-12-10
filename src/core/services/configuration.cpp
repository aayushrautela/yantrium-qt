#include "configuration.h"
#include "logging_service.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>

Configuration::Configuration(QObject* parent)
    : QObject(parent)
    , m_tmdbBaseUrl("https://api.themoviedb.org/3")
    , m_tmdbImageBaseUrl("https://image.tmdb.org/t/p/")
{
    // Get API key from compile-time define
    QString apiKey = QString::fromUtf8(TMDB_API_KEY);
    
    // If not set at compile time, try environment variable
    if (apiKey.isEmpty()) {
        apiKey = QString::fromLocal8Bit(qgetenv("TMDB_API_KEY"));
    }
    
    // If still empty, try reading from config file in app data directory
    if (apiKey.isEmpty()) {
        QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir dir;
        if (dir.exists(dataDir)) {
            QString configFile = dataDir + "/tmdb_config.txt";
            QFile file(configFile);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                apiKey = in.readLine().trimmed();
                file.close();
            }
        }
    }
    
    m_tmdbApiKey = apiKey;
    
    if (m_tmdbApiKey.isEmpty()) {
        LoggingService::logWarning("Configuration", "TMDB API key not set. Set it via CMake: -DTMDB_API_KEY=your_key");
        LoggingService::logWarning("Configuration", "Or set environment variable: TMDB_API_KEY=your_key");
        LoggingService::logWarning("Configuration", "Or create file: ~/.local/share/Yantrium/tmdb_config.txt with your API key");
    } else {
        LoggingService::logDebug("Configuration", QString("TMDB API key loaded (length: %1)").arg(m_tmdbApiKey.length()));
    }
    
    // Load OMDB API key
    QString omdbApiKey = QString::fromUtf8(OMDB_API_KEY);
    if (omdbApiKey.isEmpty()) {
        omdbApiKey = QString::fromLocal8Bit(qgetenv("OMDB_API_KEY"));
    }
    if (omdbApiKey.isEmpty()) {
        QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir dir;
        if (dir.exists(dataDir)) {
            QString configFile = dataDir + "/omdb_config.txt";
            QFile file(configFile);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                omdbApiKey = in.readLine().trimmed();
                file.close();
            }
        }
    }
    m_omdbApiKey = omdbApiKey;
    
    if (m_omdbApiKey.isEmpty()) {
        LoggingService::logDebug("Configuration", "OMDB API key not set (optional). Additional ratings will not be available.");
        LoggingService::logDebug("Configuration", "To enable OMDB ratings, set it via CMake: -DOMDB_API_KEY=your_key");
        LoggingService::logDebug("Configuration", "Or set environment variable: OMDB_API_KEY=your_key");
        LoggingService::logDebug("Configuration", "Or create file: ~/.local/share/Yantrium/omdb_config.txt with your API key");
    } else {
        LoggingService::logDebug("Configuration", QString("OMDB API key loaded (length: %1)").arg(m_omdbApiKey.length()));
    }
    
    // Load Trakt client ID
    QString clientId = QString::fromUtf8(TRAKT_CLIENT_ID);
    if (clientId.isEmpty()) {
        clientId = QString::fromLocal8Bit(qgetenv("TRAKT_CLIENT_ID"));
    }
    m_traktClientId = clientId;
    
    // Load Trakt client secret
    QString clientSecret = QString::fromUtf8(TRAKT_CLIENT_SECRET);
    if (clientSecret.isEmpty()) {
        clientSecret = QString::fromLocal8Bit(qgetenv("TRAKT_CLIENT_SECRET"));
    }
    m_traktClientSecret = clientSecret;
    
    if (!isTraktConfigured()) {
        LoggingService::logWarning("Configuration", "Trakt API not configured. Set it via CMake: -DTRAKT_CLIENT_ID=your_id -DTRAKT_CLIENT_SECRET=your_secret");
        LoggingService::logWarning("Configuration", "Or set environment variables: TRAKT_CLIENT_ID and TRAKT_CLIENT_SECRET");
    } else {
        LoggingService::logDebug("Configuration", QString("Trakt API configured (client ID length: %1)").arg(m_traktClientId.length()));
    }
}

Configuration::~Configuration()
{
}

QString Configuration::tmdbApiKey() const
{
    return m_tmdbApiKey;
}

QString Configuration::tmdbBaseUrl() const
{
    return m_tmdbBaseUrl;
}

QString Configuration::tmdbImageBaseUrl() const
{
    return m_tmdbImageBaseUrl;
}

QString Configuration::omdbApiKey() const
{
    return m_omdbApiKey;
}

bool Configuration::saveOmdbApiKey(const QString& apiKey)
{
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir;
    if (!dir.exists(dataDir)) {
        if (!dir.mkpath(dataDir)) {
            LoggingService::logError("Configuration", QString("Failed to create data directory: %1").arg(dataDir));
            return false;
        }
    }
    
    QString configFile = dataDir + "/omdb_config.txt";
    QFile file(configFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LoggingService::logError("Configuration", QString("Failed to open config file for writing: %1").arg(configFile));
        return false;
    }
    
    QTextStream out(&file);
    out << apiKey.trimmed();
    file.close();
    
    // Reload the key
    reloadOmdbApiKey();
    
    // Emit signal for QML
    emit omdbApiKeyChanged();
    
    LoggingService::logDebug("Configuration", QString("OMDB API key saved successfully (length: %1)").arg(m_omdbApiKey.length()));
    return true;
}

void Configuration::reloadOmdbApiKey()
{
    QString omdbApiKey = QString::fromUtf8(OMDB_API_KEY);
    if (omdbApiKey.isEmpty()) {
        omdbApiKey = QString::fromLocal8Bit(qgetenv("OMDB_API_KEY"));
    }
    if (omdbApiKey.isEmpty()) {
        QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir dir;
        if (dir.exists(dataDir)) {
            QString configFile = dataDir + "/omdb_config.txt";
            QFile file(configFile);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                omdbApiKey = in.readLine().trimmed();
                file.close();
            }
        }
    }
    m_omdbApiKey = omdbApiKey;
}

QString Configuration::traktClientId() const
{
    return m_traktClientId;
}

QString Configuration::traktClientSecret() const
{
    return m_traktClientSecret;
}

QString Configuration::traktRedirectUri() const
{
    return QStringLiteral("yantrium://auth/trakt");
}

QString Configuration::traktBaseUrl() const
{
    return QStringLiteral("https://api.trakt.tv");
}

QString Configuration::traktAuthUrl() const
{
    return QStringLiteral("https://trakt.tv/oauth/authorize");
}

QString Configuration::traktTokenUrl() const
{
    return QStringLiteral("https://api.trakt.tv/oauth/token");
}

QString Configuration::traktDeviceCodeUrl() const
{
    return QStringLiteral("https://api.trakt.tv/oauth/device/code");
}

QString Configuration::traktDeviceTokenUrl() const
{
    return QStringLiteral("https://api.trakt.tv/oauth/device/token");
}

QString Configuration::traktApiVersion() const
{
    return QStringLiteral("2");
}

int Configuration::defaultTraktCompletionThreshold() const
{
    return 81; // More than 80% (>80%) is considered watched
}

bool Configuration::isTraktConfigured() const
{
    return !m_traktClientId.isEmpty() && !m_traktClientSecret.isEmpty();
}

