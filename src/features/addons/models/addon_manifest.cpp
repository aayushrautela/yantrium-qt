#include "addon_manifest.h"

// Enables "string"_s and "string"_L1 literals
using namespace Qt::Literals::StringLiterals; 

AddonManifest::AddonManifest(const QString& id,
                             const QString& version,
                             const QString& name,
                             const QString& description,
                             const QJsonArray& resources,
                             const QStringList& types,
                             const QList<CatalogDefinition>& catalogs)
    : m_id(id)
    , m_version(version)
    , m_name(name)
    , m_description(description)
    , m_resources(resources)
    , m_types(types)
    , m_catalogs(catalogs)
{
}

AddonManifest AddonManifest::fromJson(const QJsonObject& json)
{
    AddonManifest manifest;
    
    // Using _L1 (Latin1StringView) is the fastest way to look up JSON keys in Qt 6
    // It avoids creating a temporary QString just to perform the lookup.
    manifest.m_id = json["id"_L1].toString();
    manifest.m_version = json["version"_L1].toString();
    manifest.m_name = json["name"_L1].toString();
    manifest.m_description = json["description"_L1].toString();
    manifest.m_resources = json["resources"_L1].toArray();
    
    // Parse types array
    if (json.contains("types"_L1) && json["types"_L1].isArray()) {
        const auto typesArray = json["types"_L1].toArray();
        manifest.m_types.reserve(typesArray.size()); // Pre-allocate memory
        for (const auto& value : typesArray) {
            manifest.m_types.append(value.toString());
        }
    }
    
    // Parse catalogs array
    if (json.contains("catalogs"_L1) && json["catalogs"_L1].isArray()) {
        const auto catalogsArray = json["catalogs"_L1].toArray();
        manifest.m_catalogs.reserve(catalogsArray.size());
        for (const auto& value : catalogsArray) {
            if (value.isObject()) {
                manifest.m_catalogs.append(CatalogDefinition::fromJson(value.toObject()));
            }
        }
    }
    
    // Parse idPrefixes
    if (json.contains("idPrefixes"_L1) && json["idPrefixes"_L1].isArray()) {
        const auto idPrefixesArray = json["idPrefixes"_L1].toArray();
        manifest.m_idPrefixes.reserve(idPrefixesArray.size());
        for (const auto& value : idPrefixesArray) {
            manifest.m_idPrefixes.append(value.toString());
        }
    }
    
    manifest.m_background = json["background"_L1].toString();
    manifest.m_logo = json["logo"_L1].toString();
    manifest.m_contactEmail = json["contactEmail"_L1].toString();
    
    if (json.contains("behaviorHints"_L1) && json["behaviorHints"_L1].isObject()) {
        manifest.m_behaviorHints = json["behaviorHints"_L1].toObject();
    }
    
    return manifest;
}

QJsonObject AddonManifest::toJson() const
{
    QJsonObject json;
    
    // We use _L1 for keys, but m_id is already a QString, so it assigns naturally
    json["id"_L1] = m_id;
    json["version"_L1] = m_version;
    json["name"_L1] = m_name;
    
    if (!m_description.isEmpty()) {
        json["description"_L1] = m_description;
    }
    
    json["resources"_L1] = m_resources;
    
    // Convert StringList to JsonArray
    // Modern Qt can often do this implicitly, but explicit conversion is safer
    json["types"_L1] = QJsonArray::fromStringList(m_types);
    
    QJsonArray catalogsArray;
    for (const auto& catalog : m_catalogs) {
        catalogsArray.append(catalog.toJson());
    }
    json["catalogs"_L1] = catalogsArray;
    
    if (!m_idPrefixes.isEmpty()) {
        json["idPrefixes"_L1] = QJsonArray::fromStringList(m_idPrefixes);
    }
    
    if (!m_background.isEmpty()) {
        json["background"_L1] = m_background;
    }
    if (!m_logo.isEmpty()) {
        json["logo"_L1] = m_logo;
    }
    if (!m_contactEmail.isEmpty()) {
        json["contactEmail"_L1] = m_contactEmail;
    }
    if (!m_behaviorHints.isEmpty()) {
        json["behaviorHints"_L1] = m_behaviorHints;
    }
    
    return json;
}

bool AddonManifest::validate(const AddonManifest& manifest)
{
    // One-liner checks are cleaner
    return !manifest.m_id.isEmpty() &&
           !manifest.m_version.isEmpty() &&
           !manifest.m_name.isEmpty() &&
           !manifest.m_resources.isEmpty() &&
           !manifest.m_types.isEmpty();
}