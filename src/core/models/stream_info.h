#ifndef STREAM_INFO_H
#define STREAM_INFO_H

#include <QString>
#include <QVariantMap>
#include <QVariantList>
#include <QJsonObject>

struct StreamInfo
{
    // Data Fields (Public by default)
    QString id;
    QString title;
    QString name;
    QString description;
    QString url;
    QString quality;
    QString type;
    QString addonId;
    QString addonName;
    QString infoHash;
    
    int fileIdx = -1;
    int size = -1;
    bool isFree = false;
    bool isDebrid = false;
    
    QVariantList subtitles;
    QVariantMap behaviorHints;

    // Default constructor is generated automatically thanks to in-class initialization
    // But if you need an explicit one taking a URL:
    StreamInfo() = default;
    explicit StreamInfo(const QString& url);

    // Methods
    QVariantMap toVariantMap() const;
    static StreamInfo fromJson(const QJsonObject& json, const QString& addonId = QString(), const QString& addonName = QString());
    static QString extractStreamUrl(const QJsonObject& json);
};

#endif // STREAM_INFO_H