#include "catalog_definition.h"

using namespace Qt::Literals::StringLiterals;

// Default constructor is now "= default" in the header, 
// so implementation is no longer needed here.

CatalogDefinition::CatalogDefinition(const QString& type,
                                     const QString& id,
                                     const QString& name,
                                     int pageSize,
                                     const QList<QJsonObject>& extra)
    : m_type(type)
    , m_id(id)
    , m_name(name)
    , m_pageSize(pageSize)
    , m_extra(extra)
{
}

CatalogDefinition CatalogDefinition::fromJson(const QJsonObject& json)
{
    CatalogDefinition catalog;
    
    // _L1 avoids creating QStrings for lookups
    catalog.m_type = json["type"_L1].toString();
    catalog.m_id = json["id"_L1].toString();
    catalog.m_name = json["name"_L1].toString();
    catalog.m_pageSize = json["pageSize"_L1].toInt(0);
    
    if (json.contains("extra"_L1) && json["extra"_L1].isArray()) {
        const auto extraArray = json["extra"_L1].toArray();
        catalog.m_extra.reserve(extraArray.size()); // Pre-allocate
        
        for (const auto& value : extraArray) {
            if (value.isObject()) {
                catalog.m_extra.append(value.toObject());
            }
        }
    }
    
    return catalog;
}

QJsonObject CatalogDefinition::toJson() const
{
    QJsonObject json;
    
    // We use _L1 for keys. 
    // m_type, m_id, etc. are implicitly shared copies (cheap).
    json["type"_L1] = m_type;
    
    if (!m_id.isEmpty()) {
        json["id"_L1] = m_id;
    }
    if (!m_name.isEmpty()) {
        json["name"_L1] = m_name;
    }
    if (m_pageSize > 0) {
        json["pageSize"_L1] = m_pageSize;
    }
    
    if (!m_extra.isEmpty()) {
        QJsonArray extraArray;
        // Unlike QStringList, QJsonArray doesn't have a helper for QList<QJsonObject>
        // so we must loop manually.
        for (const auto& obj : m_extra) {
            extraArray.append(obj);
        }
        json["extra"_L1] = extraArray;
    }
    
    return json;
}