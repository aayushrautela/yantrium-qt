#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QObject>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

// Compile-time API key definitions
#ifndef TMDB_API_KEY
#define TMDB_API_KEY ""
#endif

#ifndef OMDB_API_KEY
#define OMDB_API_KEY ""
#endif

#ifndef TRAKT_CLIENT_ID
#define TRAKT_CLIENT_ID ""
#endif

#ifndef TRAKT_CLIENT_SECRET
#define TRAKT_CLIENT_SECRET ""
#endif

class Configuration : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString tmdbApiKey READ tmdbApiKey CONSTANT)
    Q_PROPERTY(QString tmdbBaseUrl READ tmdbBaseUrl CONSTANT)
    Q_PROPERTY(QString tmdbImageBaseUrl READ tmdbImageBaseUrl CONSTANT)
    Q_PROPERTY(QString omdbApiKey READ omdbApiKey NOTIFY omdbApiKeyChanged)

public:
    explicit Configuration(QObject* parent = nullptr);
    ~Configuration();

    QString tmdbApiKey() const;
    QString tmdbBaseUrl() const;
    QString tmdbImageBaseUrl() const;
    QString omdbApiKey() const;
    Q_INVOKABLE bool saveOmdbApiKey(const QString& apiKey);
    Q_INVOKABLE void reloadOmdbApiKey();
    
    // Trakt configuration
    QString traktClientId() const;
    QString traktClientSecret() const;
    QString traktRedirectUri() const;
    QString traktBaseUrl() const;
    QString traktAuthUrl() const;
    QString traktTokenUrl() const;
    QString traktDeviceCodeUrl() const;
    QString traktDeviceTokenUrl() const;
    QString traktApiVersion() const;
    int defaultTraktCompletionThreshold() const;
    bool isTraktConfigured() const;

signals:
    void omdbApiKeyChanged();

private:
    Q_DISABLE_COPY(Configuration)

    QString m_tmdbApiKey;
    QString m_tmdbBaseUrl;
    QString m_tmdbImageBaseUrl;
    QString m_omdbApiKey;
    
    // Trakt configuration
    QString m_traktClientId;
    QString m_traktClientSecret;
};

#endif // CONFIGURATION_H

