#include "addon_config.h"
#include "core/database/addon_dao.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

AddonConfig::AddonConfig()
    : m_enabled(true)
{
}

AddonConfig::AddonConfig(const QString& id,
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
                         const QDateTime& updatedAt)
    : m_id(id)
    , m_name(name)
    , m_version(version)
    , m_description(description)
    , m_manifestUrl(manifestUrl)
    , m_baseUrl(baseUrl)
    , m_enabled(enabled)
    , m_manifestData(manifestData)
    , m_resources(resources)
    , m_types(types)
    , m_createdAt(createdAt)
    , m_updatedAt(updatedAt)
{
}

AddonConfig AddonConfig::fromDatabase(const AddonRecord& record)
{
    AddonConfig config;
    config.m_id = record.id;
    config.m_name = record.name;
    config.m_version = record.version;
    config.m_description = record.description;
    config.m_manifestUrl = record.manifestUrl;
    config.m_baseUrl = record.baseUrl;
    config.m_enabled = record.enabled;
    config.m_manifestData = record.manifestData;
    config.m_createdAt = record.createdAt;
    config.m_updatedAt = record.updatedAt;
    
    // Parse resources JSON
    QJsonDocument resourcesDoc = QJsonDocument::fromJson(record.resources.toUtf8());
    if (resourcesDoc.isArray()) {
        config.m_resources = resourcesDoc.array();
    } else {
        config.m_resources = QJsonArray();
    }
    
    // Parse types JSON
    QJsonDocument typesDoc = QJsonDocument::fromJson(record.types.toUtf8());
    if (typesDoc.isArray()) {
        QJsonArray typesArray = typesDoc.array();
        for (const QJsonValue& value : typesArray) {
            config.m_types.append(value.toString());
        }
    }
    
    return config;
}

AddonRecord AddonConfig::toDatabaseRecord() const
{
    AddonRecord record;
    record.id = m_id;
    record.name = m_name;
    record.version = m_version;
    record.description = m_description;
    record.manifestUrl = m_manifestUrl;
    record.baseUrl = m_baseUrl;
    record.enabled = m_enabled;
    record.manifestData = m_manifestData;
    record.createdAt = m_createdAt;
    record.updatedAt = m_updatedAt;
    
    // Convert resources to JSON string
    QJsonDocument resourcesDoc(m_resources);
    record.resources = QString::fromUtf8(resourcesDoc.toJson(QJsonDocument::Compact));
    
    // Convert types to JSON string
    QJsonArray typesArray;
    for (const QString& type : m_types) {
        typesArray.append(type);
    }
    QJsonDocument typesDoc(typesArray);
    record.types = QString::fromUtf8(typesDoc.toJson(QJsonDocument::Compact));
    
    return record;
}

