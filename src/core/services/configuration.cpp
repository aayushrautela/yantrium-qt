#include "configuration.h"
#include <QDebug>
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
        qWarning() << "TMDB API key not set. Set it via CMake: -DTMDB_API_KEY=your_key";
        qWarning() << "Or set environment variable: TMDB_API_KEY=your_key";
        qWarning() << "Or create file: ~/.local/share/Yantrium/tmdb_config.txt with your API key";
    } else {
        qDebug() << "TMDB API key loaded (length:" << m_tmdbApiKey.length() << ")";
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
        qWarning() << "Trakt API not configured. Set it via CMake: -DTRAKT_CLIENT_ID=your_id -DTRAKT_CLIENT_SECRET=your_secret";
        qWarning() << "Or set environment variables: TRAKT_CLIENT_ID and TRAKT_CLIENT_SECRET";
    } else {
        qDebug() << "Trakt API configured (client ID length:" << m_traktClientId.length() << ")";
    }
}

Configuration::~Configuration()
{
}

Configuration& Configuration::instance()
{
    static Configuration instance;
    return instance;
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
    return 80; // 80% completion threshold
}

bool Configuration::isTraktConfigured() const
{
    return !m_traktClientId.isEmpty() && !m_traktClientSecret.isEmpty();
}

