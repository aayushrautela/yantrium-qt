#include "addon_dao.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

AddonDao::AddonDao(QSqlDatabase database)
    : m_database(database)
{
}

bool AddonDao::insertAddon(const AddonRecord& addon)
{
    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT INTO addons (
            id, name, version, description, manifestUrl, baseUrl,
            enabled, manifestData, resources, types, createdAt, updatedAt
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    
    query.addBindValue(addon.id);
    query.addBindValue(addon.name);
    query.addBindValue(addon.version);
    query.addBindValue(addon.description);
    query.addBindValue(addon.manifestUrl);
    query.addBindValue(addon.baseUrl);
    query.addBindValue(addon.enabled ? 1 : 0);
    query.addBindValue(addon.manifestData);
    query.addBindValue(addon.resources);
    query.addBindValue(addon.types);
    query.addBindValue(addon.createdAt.toString(Qt::ISODate));
    query.addBindValue(addon.updatedAt.toString(Qt::ISODate));
    
    if (!query.exec()) {
        qWarning() << "Failed to insert addon:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool AddonDao::updateAddon(const AddonRecord& addon)
{
    QSqlQuery query(m_database);
    query.prepare(R"(
        UPDATE addons SET
            name = ?, version = ?, description = ?, manifestUrl = ?,
            baseUrl = ?, enabled = ?, manifestData = ?, resources = ?,
            types = ?, updatedAt = ?
        WHERE id = ?
    )");
    
    query.addBindValue(addon.name);
    query.addBindValue(addon.version);
    query.addBindValue(addon.description);
    query.addBindValue(addon.manifestUrl);
    query.addBindValue(addon.baseUrl);
    query.addBindValue(addon.enabled ? 1 : 0);
    query.addBindValue(addon.manifestData);
    query.addBindValue(addon.resources);
    query.addBindValue(addon.types);
    query.addBindValue(addon.updatedAt.toString(Qt::ISODate));
    query.addBindValue(addon.id);
    
    if (!query.exec()) {
        qWarning() << "Failed to update addon:" << query.lastError().text();
        return false;
    }
    
    return query.numRowsAffected() > 0;
}

std::unique_ptr<AddonRecord> AddonDao::getAddonById(const QString& id)
{
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM addons WHERE id = ?");
    query.addBindValue(id);
    
    if (!query.exec()) {
        qWarning() << "Failed to get addon:" << query.lastError().text();
        return nullptr;
    }
    
    if (!query.next()) {
        return nullptr;
    }
    
    return std::make_unique<AddonRecord>(recordFromQuery(query));
}

QList<AddonRecord> AddonDao::getAllAddons()
{
    QList<AddonRecord> addons;
    QSqlQuery query(m_database);
    
    if (!query.exec("SELECT * FROM addons ORDER BY name")) {
        qWarning() << "Failed to get all addons:" << query.lastError().text();
        return addons;
    }
    
    while (query.next()) {
        addons.append(recordFromQuery(query));
    }
    
    return addons;
}

QList<AddonRecord> AddonDao::getEnabledAddons()
{
    QList<AddonRecord> addons;
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM addons WHERE enabled = 1 ORDER BY name");
    
    if (!query.exec()) {
        qWarning() << "Failed to get enabled addons:" << query.lastError().text();
        return addons;
    }
    
    while (query.next()) {
        addons.append(recordFromQuery(query));
    }
    
    return addons;
}

bool AddonDao::deleteAddon(const QString& id)
{
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM addons WHERE id = ?");
    query.addBindValue(id);
    
    if (!query.exec()) {
        qWarning() << "Failed to delete addon:" << query.lastError().text();
        return false;
    }
    
    return query.numRowsAffected() > 0;
}

bool AddonDao::toggleAddonEnabled(const QString& id, bool enabled)
{
    QSqlQuery query(m_database);
    query.prepare("UPDATE addons SET enabled = ? WHERE id = ?");
    query.addBindValue(enabled ? 1 : 0);
    query.addBindValue(id);
    
    if (!query.exec()) {
        qWarning() << "Failed to toggle addon enabled:" << query.lastError().text();
        return false;
    }
    
    return query.numRowsAffected() > 0;
}

AddonRecord AddonDao::recordFromQuery(const QSqlQuery& query)
{
    AddonRecord record;
    record.id = query.value("id").toString();
    record.name = query.value("name").toString();
    record.version = query.value("version").toString();
    record.description = query.value("description").toString();
    record.manifestUrl = query.value("manifestUrl").toString();
    record.baseUrl = query.value("baseUrl").toString();
    record.enabled = query.value("enabled").toInt() == 1;
    record.manifestData = query.value("manifestData").toString();
    record.resources = query.value("resources").toString();
    record.types = query.value("types").toString();
    record.createdAt = QDateTime::fromString(query.value("createdAt").toString(), Qt::ISODate);
    record.updatedAt = QDateTime::fromString(query.value("updatedAt").toString(), Qt::ISODate);
    return record;
}


