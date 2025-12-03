#include "stream_info.h"
#include <QJsonArray>
#include <QUrl>
#include <QDebug>

StreamInfo::StreamInfo()
    : m_fileIdx(-1)
    , m_size(-1)
    , m_isFree(false)
    , m_isDebrid(false)
{
}

StreamInfo::StreamInfo(const QString& url)
    : m_url(url)
    , m_fileIdx(-1)
    , m_size(-1)
    , m_isFree(false)
    , m_isDebrid(false)
{
}

QVariantMap StreamInfo::toVariantMap() const
{
    QVariantMap map;
    map["id"] = m_id;
    map["title"] = m_title;
    map["name"] = m_name;
    map["description"] = m_description;
    map["url"] = m_url;
    map["quality"] = m_quality;
    map["type"] = m_type;
    map["addonId"] = m_addonId;
    map["addonName"] = m_addonName;
    map["infoHash"] = m_infoHash;
    map["fileIdx"] = m_fileIdx >= 0 ? m_fileIdx : QVariant();
    map["size"] = m_size >= 0 ? m_size : QVariant();
    map["isFree"] = m_isFree;
    map["isDebrid"] = m_isDebrid;
    map["subtitles"] = m_subtitles;
    map["behaviorHints"] = m_behaviorHints;
    return map;
}

QString StreamInfo::extractStreamUrl(const QJsonObject& json)
{
    // Prefer plain string URLs
    if (json["url"].isString()) {
        return json["url"].toString();
    }
    
    // Some addons might nest the URL inside an object
    if (json["url"].isObject()) {
        QJsonObject urlObj = json["url"].toObject();
        if (urlObj.contains("url") && urlObj["url"].isString()) {
            return urlObj["url"].toString();
        }
    }
    
    // Handle magnet links from infoHash
    if (json.contains("infoHash") && json["infoHash"].isString()) {
        QString infoHash = json["infoHash"].toString();
        QStringList trackers = {
            "udp://tracker.opentrackr.org:1337/announce",
            "udp://9.rarbg.com:2810/announce",
            "udp://tracker.openbittorrent.com:6969/announce",
            "udp://tracker.torrent.eu.org:451/announce",
            "udp://open.stealth.si:80/announce",
            "udp://tracker.leechers-paradise.org:6969/announce",
            "udp://tracker.coppersurfer.tk:6969/announce",
            "udp://tracker.internetwarriors.net:1337/announce"
        };
        
        QString trackersString;
        for (const QString& tracker : trackers) {
            trackersString += "&tr=" + QUrl::toPercentEncoding(tracker);
        }
        
        QString title = json["title"].toString();
        if (title.isEmpty()) {
            title = json["name"].toString();
        }
        if (title.isEmpty()) {
            title = "Unknown";
        }
        
        QString encodedTitle = QUrl::toPercentEncoding(title);
        return QString("magnet:?xt=urn:btih:%1&dn=%2%3").arg(infoHash, encodedTitle, trackersString);
    }
    
    return QString();
}

StreamInfo StreamInfo::fromJson(const QJsonObject& json, const QString& addonId, const QString& addonName)
{
    StreamInfo info;
    
    // Extract URL first (required)
    QString streamUrl = extractStreamUrl(json);
    info.setUrl(streamUrl);
    
    info.setId(json["id"].toString());
    info.setTitle(json["title"].toString());
    info.setName(json["name"].toString());
    info.setDescription(json["description"].toString());
    info.setQuality(json["quality"].toString());
    info.setType(json["type"].toString());
    info.setAddonId(addonId.isEmpty() ? json["addonId"].toString() : addonId);
    info.setAddonName(addonName.isEmpty() ? json["addonName"].toString() : addonName);
    info.setInfoHash(json["infoHash"].toString());
    info.setFileIdx(json["fileIdx"].toInt(-1));
    info.setSize(json["size"].toInt(-1));
    info.setIsFree(json["isFree"].toBool(false));
    info.setIsDebrid(json["isDebrid"].toBool(false));
    
    // Parse subtitles
    if (json.contains("subtitles") && json["subtitles"].isArray()) {
        QVariantList subtitles;
        QJsonArray subtitlesArray = json["subtitles"].toArray();
        for (const QJsonValue& value : subtitlesArray) {
            if (value.isObject()) {
                QJsonObject subObj = value.toObject();
                QVariantMap subMap;
                subMap["url"] = subObj["url"].toString();
                subMap["lang"] = subObj["lang"].toString();
                subMap["id"] = subObj["id"].toString();
                subtitles.append(subMap);
            }
        }
        info.setSubtitles(subtitles);
    }
    
    // Parse behaviorHints
    if (json.contains("behaviorHints") && json["behaviorHints"].isObject()) {
        QVariantMap hints;
        QJsonObject hintsObj = json["behaviorHints"].toObject();
        for (auto it = hintsObj.begin(); it != hintsObj.end(); ++it) {
            hints[it.key()] = it.value().toVariant();
        }
        info.setBehaviorHints(hints);
    }
    
    return info;
}

