#ifndef STREAM_INFO_H
#define STREAM_INFO_H

#include <QString>
#include <QVariantMap>
#include <QVariantList>
#include <QJsonObject>

class StreamInfo
{
public:
    StreamInfo();
    StreamInfo(const QString& url);
    
    // Required
    QString url() const { return m_url; }
    void setUrl(const QString& url) { m_url = url; }
    
    // Optional
    QString id() const { return m_id; }
    void setId(const QString& id) { m_id = id; }
    
    QString title() const { return m_title; }
    void setTitle(const QString& title) { m_title = title; }
    
    QString name() const { return m_name; }
    void setName(const QString& name) { m_name = name; }
    
    QString description() const { return m_description; }
    void setDescription(const QString& description) { m_description = description; }
    
    QString quality() const { return m_quality; }
    void setQuality(const QString& quality) { m_quality = quality; }
    
    QString type() const { return m_type; }
    void setType(const QString& type) { m_type = type; }
    
    QString addonId() const { return m_addonId; }
    void setAddonId(const QString& addonId) { m_addonId = addonId; }
    
    QString addonName() const { return m_addonName; }
    void setAddonName(const QString& addonName) { m_addonName = addonName; }
    
    QString infoHash() const { return m_infoHash; }
    void setInfoHash(const QString& infoHash) { m_infoHash = infoHash; }
    
    int fileIdx() const { return m_fileIdx; }
    void setFileIdx(int fileIdx) { m_fileIdx = fileIdx; }
    
    int size() const { return m_size; }
    void setSize(int size) { m_size = size; }
    
    bool isFree() const { return m_isFree; }
    void setIsFree(bool isFree) { m_isFree = isFree; }
    
    bool isDebrid() const { return m_isDebrid; }
    void setIsDebrid(bool isDebrid) { m_isDebrid = isDebrid; }
    
    QVariantList subtitles() const { return m_subtitles; }
    void setSubtitles(const QVariantList& subtitles) { m_subtitles = subtitles; }
    
    QVariantMap behaviorHints() const { return m_behaviorHints; }
    void setBehaviorHints(const QVariantMap& hints) { m_behaviorHints = hints; }
    
    // Convert to QVariantMap for QML
    QVariantMap toVariantMap() const;
    
    // Parse from JSON
    static StreamInfo fromJson(const QJsonObject& json, const QString& addonId = QString(), const QString& addonName = QString());
    
    // Extract stream URL from various formats
    static QString extractStreamUrl(const QJsonObject& json);

private:
    QString m_id;
    QString m_title;
    QString m_name;
    QString m_description;
    QString m_url;
    QString m_quality;
    QString m_type;
    QString m_addonId;
    QString m_addonName;
    QString m_infoHash;
    int m_fileIdx;
    int m_size;
    bool m_isFree;
    bool m_isDebrid;
    QVariantList m_subtitles;
    QVariantMap m_behaviorHints;
};

#endif // STREAM_INFO_H

