#include "catalog_preferences_dao.h"
#include "database_manager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QVariant>

CatalogPreferencesDao::CatalogPreferencesDao()
    : m_database(QSqlDatabase::database(DatabaseManager::CONNECTION_NAME))
{
}

bool CatalogPreferencesDao::upsertPreference(const CatalogPreferenceRecord& preference)
{
    // This query uses SQLite 'ON CONFLICT' to handle upserts atomically.
    // It requires a UNIQUE constraint on (addon_id, catalog_type, catalog_id).
    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT INTO catalog_preferences (
            addon_id, catalog_type, catalog_id,
            enabled, is_hero_source, created_at, updated_at
        ) VALUES (?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(addon_id, catalog_type, catalog_id)
        DO UPDATE SET
            enabled = excluded.enabled,
            is_hero_source = excluded.is_hero_source,
            updated_at = excluded.updated_at
    )");

    QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    // Bind values
    query.addBindValue(preference.addonId);
    query.addBindValue(preference.catalogType);
    query.addBindValue(normalizeId(preference.catalogId));
    query.addBindValue(preference.enabled ? 1 : 0);
    query.addBindValue(preference.isHeroSource ? 1 : 0);
    query.addBindValue(now); // Created At
    query.addBindValue(now); // Updated At

    if (!query.exec()) {
        qWarning() << "Failed to upsert catalog preference:" << query.lastError().text();
        return false;
    }

    return true;
}

std::unique_ptr<CatalogPreferenceRecord> CatalogPreferencesDao::getPreference(
    const QString& addonId,
    const QString& catalogType,
    const QString& catalogId)
{
    QSqlQuery query(m_database);
    query.prepare(R"(
        SELECT * FROM catalog_preferences
        WHERE addon_id = ? AND catalog_type = ? AND catalog_id = ?
    )");
    query.addBindValue(addonId);
    query.addBindValue(catalogType);
    query.addBindValue(normalizeId(catalogId));

    if (!query.exec()) {
        qWarning() << "Failed to get catalog preference:" << query.lastError().text();
        return nullptr;
    }

    if (!query.next()) {
        return nullptr;
    }

    return std::make_unique<CatalogPreferenceRecord>(recordFromQuery(query));
}

QList<CatalogPreferenceRecord> CatalogPreferencesDao::getAllPreferences()
{
    QList<CatalogPreferenceRecord> preferences;
    QSqlQuery query(m_database);

    if (!query.exec("SELECT * FROM catalog_preferences ORDER BY addon_id, catalog_type")) {
        qWarning() << "Failed to get all catalog preferences:" << query.lastError().text();
        return preferences;
    }

    while (query.next()) {
        preferences.append(recordFromQuery(query));
    }

    return preferences;
}

bool CatalogPreferencesDao::toggleCatalogEnabled(
    const QString& addonId,
    const QString& catalogType,
    const QString& catalogId,
    bool enabled)
{
    QSqlQuery query(m_database);
    
    // Attempt update first
    query.prepare(R"(
        UPDATE catalog_preferences SET enabled = ?, updated_at = ?
        WHERE addon_id = ? AND catalog_type = ? AND catalog_id = ?
    )");
    query.addBindValue(enabled ? 1 : 0);
    query.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    query.addBindValue(addonId);
    query.addBindValue(catalogType);
    query.addBindValue(normalizeId(catalogId));

    if (!query.exec()) {
        qWarning() << "Failed to toggle catalog enabled:" << query.lastError().text();
        return false;
    }

    // If row doesn't exist, create it with defaults
    if (query.numRowsAffected() == 0) {
        CatalogPreferenceRecord preference;
        preference.addonId = addonId;
        preference.catalogType = catalogType;
        preference.catalogId = catalogId;
        preference.enabled = enabled;
        // Default hero source to false for new toggles
        preference.isHeroSource = false; 
        return upsertPreference(preference);
    }

    return true;
}

bool CatalogPreferencesDao::setHeroCatalog(
    const QString& addonId,
    const QString& catalogType,
    const QString& catalogId)
{
    // SUPPORT FOR MULTIPLE HEROES:
    // We strictly update ONLY this specific record. We do not unset others.
    
    QSqlQuery query(m_database);
    query.prepare(R"(
        UPDATE catalog_preferences SET is_hero_source = 1, updated_at = ?
        WHERE addon_id = ? AND catalog_type = ? AND catalog_id = ?
    )");
    query.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    query.addBindValue(addonId);
    query.addBindValue(catalogType);
    query.addBindValue(normalizeId(catalogId));

    if (!query.exec()) {
        qWarning() << "Failed to set hero catalog:" << query.lastError().text();
        return false;
    }

    // If row doesn't exist, create it
    if (query.numRowsAffected() == 0) {
        CatalogPreferenceRecord preference;
        preference.addonId = addonId;
        preference.catalogType = catalogType;
        preference.catalogId = catalogId;
        preference.isHeroSource = true;
        // If we are explicitly setting it as hero, we likely want it enabled too
        preference.enabled = true; 
        return upsertPreference(preference);
    }

    return true;
}

bool CatalogPreferencesDao::unsetHeroCatalog(
    const QString& addonId,
    const QString& catalogType,
    const QString& catalogId)
{
    QSqlQuery query(m_database);
    query.prepare(R"(
        UPDATE catalog_preferences SET is_hero_source = 0, updated_at = ?
        WHERE addon_id = ? AND catalog_type = ? AND catalog_id = ?
    )");
    query.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    query.addBindValue(addonId);
    query.addBindValue(catalogType);
    query.addBindValue(normalizeId(catalogId));

    if (!query.exec()) {
        qWarning() << "Failed to unset hero catalog:" << query.lastError().text();
        return false;
    }

    return true;
}

QList<CatalogPreferenceRecord> CatalogPreferencesDao::getHeroCatalogs()
{
    QList<CatalogPreferenceRecord> preferences;
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM catalog_preferences WHERE is_hero_source = 1 ORDER BY addon_id, catalog_type");

    if (!query.exec()) {
        qWarning() << "Failed to get hero catalogs:" << query.lastError().text();
        return preferences;
    }

    while (query.next()) {
        preferences.append(recordFromQuery(query));
    }

    return preferences;
}

CatalogPreferenceRecord CatalogPreferencesDao::recordFromQuery(const QSqlQuery& query)
{
    CatalogPreferenceRecord record;
    record.addonId = query.value("addon_id").toString();
    record.catalogType = query.value("catalog_type").toString();
    record.catalogId = query.value("catalog_id").toString();
    record.enabled = query.value("enabled").toInt() == 1;
    record.isHeroSource = query.value("is_hero_source").toInt() == 1;
    record.createdAt = QDateTime::fromString(query.value("created_at").toString(), Qt::ISODate);
    record.updatedAt = QDateTime::fromString(query.value("updated_at").toString(), Qt::ISODate);
    return record;
}

QString CatalogPreferencesDao::normalizeId(const QString& id) const
{
    return id;  // Database schema handles empty strings with DEFAULT ''
}