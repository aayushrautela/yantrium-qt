#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QObject>
#include <QString>

// Compile-time API key definition
#ifndef TMDB_API_KEY
#define TMDB_API_KEY ""
#endif

class Configuration : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString tmdbApiKey READ tmdbApiKey CONSTANT)
    Q_PROPERTY(QString tmdbBaseUrl READ tmdbBaseUrl CONSTANT)
    Q_PROPERTY(QString tmdbImageBaseUrl READ tmdbImageBaseUrl CONSTANT)

public:
    static Configuration& instance();

    QString tmdbApiKey() const;
    QString tmdbBaseUrl() const;
    QString tmdbImageBaseUrl() const;

private:
    explicit Configuration(QObject* parent = nullptr);
    ~Configuration();
    Q_DISABLE_COPY(Configuration)

    QString m_tmdbApiKey;
    QString m_tmdbBaseUrl;
    QString m_tmdbImageBaseUrl;
};

#endif // CONFIGURATION_H

