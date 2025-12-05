#include "stream_info.h"
#include <QJsonArray>
#include <QUrl>
#include <QDebug>

// Constructor with URL
StreamInfo::StreamInfo(const QString& urlIn)
    : url(urlIn)
{
}

QVariantMap StreamInfo::toVariantMap() const
{
    QVariantMap map;
    map["id"] = id;
    map["title"] = title;
    map["name"] = name;
    map["description"] = description;
    map["url"] = url;
    map["quality"] = quality;
    map["type"] = type;
    map["addonId"] = addonId;
    map["addonName"] = addonName;
    map["infoHash"] = infoHash;
    
    // Only add if valid, otherwise QVariant() (null)
    map["fileIdx"] = (fileIdx >= 0) ? fileIdx : QVariant();
    map["size"] = (size >= 0) ? size : QVariant();
    
    map["isFree"] = isFree;
    map["isDebrid"] = isDebrid;
    map["subtitles"] = subtitles;
    map["behaviorHints"] = behaviorHints;
    return map;
}

QString StreamInfo::extractStreamUrl(const QJsonObject& json)
{
    // 1. Prefer plain string URLs
    if (json["url"].isString()) {
        return json["url"].toString();
    }
    
    // 2. Some addons might nest the URL inside an object
    if (json["url"].isObject()) {
        const QJsonObject urlObj = json["url"].toObject();
        if (urlObj.contains("url") && urlObj["url"].isString()) {
            return urlObj["url"].toString();
        }
    }
    
    // 3. Handle magnet links from infoHash
    if (json.contains("infoHash") && json["infoHash"].isString()) {
        const QString hash = json["infoHash"].toString();
        
        // Static list to avoid recreating it every function call
        static const QStringList trackers = {
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
        
        QString titleStr = json["title"].toString();
        if (titleStr.isEmpty()) titleStr = json["name"].toString();
        if (titleStr.isEmpty()) titleStr = "Unknown";
        
        return QString("magnet:?xt=urn:btih:%1&dn=%2%3")
               .arg(hash, QUrl::toPercentEncoding(titleStr), trackersString);
    }
    
    return QString();
}

StreamInfo StreamInfo::fromJson(const QJsonObject& json, const QString& defaultAddonId, const QString& defaultAddonName)
{
    StreamInfo info;
    
    // Extract URL first
    info.url = extractStreamUrl(json);
    
    // Assign fields directly
    info.id = json["id"].toString();
    info.title = json["title"].toString();
    info.name = json["name"].toString();
    info.description = json["description"].toString();
    info.quality = json["quality"].toString();
    info.type = json["type"].toString();
    
    info.addonId = defaultAddonId.isEmpty() ? json["addonId"].toString() : defaultAddonId;
    info.addonName = defaultAddonName.isEmpty() ? json["addonName"].toString() : defaultAddonName;
    
    info.infoHash = json["infoHash"].toString();
    info.fileIdx = json["fileIdx"].toInt(-1);
    info.size = json["size"].toInt(-1);
    info.isFree = json["isFree"].toBool(false);
    info.isDebrid = json["isDebrid"].toBool(false);
    
    // Parse subtitles
    if (json.contains("subtitles") && json["subtitles"].isArray()) {
        const QJsonArray subtitlesArray = json["subtitles"].toArray();
        for (const QJsonValue& value : subtitlesArray) {
            if (value.isObject()) {
                const QJsonObject subObj = value.toObject();
                QVariantMap subMap;
                subMap["url"] = subObj["url"].toString();
                subMap["lang"] = subObj["lang"].toString();
                subMap["id"] = subObj["id"].toString();
                info.subtitles.append(subMap);
            }
        }
    }
    
    // Parse behaviorHints
    if (json.contains("behaviorHints") && json["behaviorHints"].isObject()) {
        const QJsonObject hintsObj = json["behaviorHints"].toObject();
        for (auto it = hintsObj.begin(); it != hintsObj.end(); ++it) {
            info.behaviorHints[it.key()] = it.value().toVariant();
        }
    }
    
    return info;
}