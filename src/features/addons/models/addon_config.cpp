#include "addon_config.h"
#include "core/database/addon_dao.h" 
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariant>

AddonConfig AddonConfig::fromDatabase(const AddonRecord& record)
{
    AddonConfig config;
    
    // Direct assignment
    config.id = record.id;
    config.name = record.name;
    config.version = record.version;
    config.description = record.description;
    config.manifestUrl = record.manifestUrl;
    config.baseUrl = record.baseUrl;
    config.enabled = record.enabled;
    config.manifestData = record.manifestData;
    config.createdAt = record.createdAt;
    config.updatedAt = record.updatedAt;

    // Modern JSON parsing
    // 1. Resources: Parse directly to Array
    const QByteArray resourcesJson = record.resources.toUtf8();
    if (!resourcesJson.isEmpty()) {
        config.resources = QJsonDocument::fromJson(resourcesJson).array();
    }

    // 2. Types: Parse manually to ensure safety
    const QByteArray typesJson = record.types.toUtf8();
    if (!typesJson.isEmpty()) {
        QJsonArray arr = QJsonDocument::fromJson(typesJson).array();
        for (const auto& val : arr) {
            config.types.append(val.toString());
        }
    }

    return config;
}

AddonRecord AddonConfig::toDatabaseRecord() const
{
    AddonRecord record;

    record.id = id;
    record.name = name;
    record.version = version;
    record.description = description;
    record.manifestUrl = manifestUrl;
    record.baseUrl = baseUrl;
    record.enabled = enabled;
    record.manifestData = manifestData;
    record.createdAt = createdAt;
    record.updatedAt = updatedAt;

    // Modern JSON Serialization
    
    // 1. Resources -> JSON String
    record.resources = QString::fromUtf8(
        QJsonDocument(resources).toJson(QJsonDocument::Compact)
    );

    // 2. StringList -> JSON String
    record.types = QString::fromUtf8(
        QJsonDocument(QJsonArray::fromStringList(types)).toJson(QJsonDocument::Compact)
    );

    return record;
}