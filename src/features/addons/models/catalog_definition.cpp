#include "catalog_definition.h"
#include <QJsonArray>

CatalogDefinition::CatalogDefinition()
    : m_pageSize(0)
{
}

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
    catalog.m_type = json["type"].toString();
    catalog.m_id = json["id"].toString();
    catalog.m_name = json["name"].toString();
    catalog.m_pageSize = json["pageSize"].toInt(0);
    
    if (json.contains("extra") && json["extra"].isArray()) {
        QJsonArray extraArray = json["extra"].toArray();
        for (const QJsonValue& value : extraArray) {
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
    json["type"] = m_type;
    if (!m_id.isEmpty()) {
        json["id"] = m_id;
    }
    if (!m_name.isEmpty()) {
        json["name"] = m_name;
    }
    if (m_pageSize > 0) {
        json["pageSize"] = m_pageSize;
    }
    if (!m_extra.isEmpty()) {
        QJsonArray extraArray;
        for (const QJsonObject& obj : m_extra) {
            extraArray.append(obj);
        }
        json["extra"] = extraArray;
    }
    return json;
}






