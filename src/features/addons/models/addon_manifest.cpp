#include "addon_manifest.h"
#include <QJsonArray>

AddonManifest::AddonManifest()
{
}

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
    manifest.m_id = json["id"].toString();
    manifest.m_version = json["version"].toString();
    manifest.m_name = json["name"].toString();
    manifest.m_description = json["description"].toString();
    manifest.m_resources = json["resources"].toArray();
    
    // Parse types array
    if (json.contains("types") && json["types"].isArray()) {
        QJsonArray typesArray = json["types"].toArray();
        for (const QJsonValue& value : typesArray) {
            manifest.m_types.append(value.toString());
        }
    }
    
    // Parse catalogs array
    if (json.contains("catalogs") && json["catalogs"].isArray()) {
        QJsonArray catalogsArray = json["catalogs"].toArray();
        for (const QJsonValue& value : catalogsArray) {
            if (value.isObject()) {
                manifest.m_catalogs.append(CatalogDefinition::fromJson(value.toObject()));
            }
        }
    }
    
    // Parse idPrefixes
    if (json.contains("idPrefixes") && json["idPrefixes"].isArray()) {
        QJsonArray idPrefixesArray = json["idPrefixes"].toArray();
        for (const QJsonValue& value : idPrefixesArray) {
            manifest.m_idPrefixes.append(value.toString());
        }
    }
    
    manifest.m_background = json["background"].toString();
    manifest.m_logo = json["logo"].toString();
    manifest.m_contactEmail = json["contactEmail"].toString();
    
    if (json.contains("behaviorHints") && json["behaviorHints"].isObject()) {
        manifest.m_behaviorHints = json["behaviorHints"].toObject();
    }
    
    return manifest;
}

QJsonObject AddonManifest::toJson() const
{
    QJsonObject json;
    json["id"] = m_id;
    json["version"] = m_version;
    json["name"] = m_name;
    if (!m_description.isEmpty()) {
        json["description"] = m_description;
    }
    json["resources"] = m_resources;
    
    QJsonArray typesArray;
    for (const QString& type : m_types) {
        typesArray.append(type);
    }
    json["types"] = typesArray;
    
    QJsonArray catalogsArray;
    for (const CatalogDefinition& catalog : m_catalogs) {
        catalogsArray.append(catalog.toJson());
    }
    json["catalogs"] = catalogsArray;
    
    if (!m_idPrefixes.isEmpty()) {
        QJsonArray idPrefixesArray;
        for (const QString& prefix : m_idPrefixes) {
            idPrefixesArray.append(prefix);
        }
        json["idPrefixes"] = idPrefixesArray;
    }
    
    if (!m_background.isEmpty()) {
        json["background"] = m_background;
    }
    if (!m_logo.isEmpty()) {
        json["logo"] = m_logo;
    }
    if (!m_contactEmail.isEmpty()) {
        json["contactEmail"] = m_contactEmail;
    }
    if (!m_behaviorHints.isEmpty()) {
        json["behaviorHints"] = m_behaviorHints;
    }
    
    return json;
}

bool AddonManifest::validate(const AddonManifest& manifest)
{
    if (manifest.m_id.isEmpty()) return false;
    if (manifest.m_version.isEmpty()) return false;
    if (manifest.m_name.isEmpty()) return false;
    if (manifest.m_resources.isEmpty()) return false;
    if (manifest.m_types.isEmpty()) return false;
    return true;
}

