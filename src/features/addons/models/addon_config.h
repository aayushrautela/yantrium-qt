#ifndef ADDON_CONFIG_H
#define ADDON_CONFIG_H

#include <QString>
#include <QDateTime>
#include <QJsonArray>
#include <QStringList>
#include "addon_manifest.h"

// Forward declaration
struct AddonRecord;

class AddonConfig
{
public:
    AddonConfig();
    AddonConfig(const QString& id,
                const QString& name,
                const QString& version,
                const QString& description,
                const QString& manifestUrl,
                const QString& baseUrl,
                bool enabled,
                const QString& manifestData,
                const QJsonArray& resources,
                const QStringList& types,
                const QDateTime& createdAt,
                const QDateTime& updatedAt);
    
    static AddonConfig fromDatabase(const AddonRecord& record);
    AddonRecord toDatabaseRecord() const;
    
    QString id() const { return m_id; }
    void setId(const QString& id) { m_id = id; }
    
    QString name() const { return m_name; }
    void setName(const QString& name) { m_name = name; }
    
    QString version() const { return m_version; }
    void setVersion(const QString& version) { m_version = version; }
    
    QString description() const { return m_description; }
    void setDescription(const QString& description) { m_description = description; }
    
    QString manifestUrl() const { return m_manifestUrl; }
    void setManifestUrl(const QString& manifestUrl) { m_manifestUrl = manifestUrl; }
    
    QString baseUrl() const { return m_baseUrl; }
    void setBaseUrl(const QString& baseUrl) { m_baseUrl = baseUrl; }
    
    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled) { m_enabled = enabled; }
    
    QString manifestData() const { return m_manifestData; }
    void setManifestData(const QString& manifestData) { m_manifestData = manifestData; }
    
    QJsonArray resources() const { return m_resources; }
    void setResources(const QJsonArray& resources) { m_resources = resources; }
    
    QStringList types() const { return m_types; }
    void setTypes(const QStringList& types) { m_types = types; }
    
    QDateTime createdAt() const { return m_createdAt; }
    void setCreatedAt(const QDateTime& createdAt) { m_createdAt = createdAt; }
    
    QDateTime updatedAt() const { return m_updatedAt; }
    void setUpdatedAt(const QDateTime& updatedAt) { m_updatedAt = updatedAt; }
    
private:
    QString m_id;
    QString m_name;
    QString m_version;
    QString m_description;
    QString m_manifestUrl;
    QString m_baseUrl;
    bool m_enabled;
    QString m_manifestData;
    QJsonArray m_resources;
    QStringList m_types;
    QDateTime m_createdAt;
    QDateTime m_updatedAt;
};

#endif // ADDON_CONFIG_H

