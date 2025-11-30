#include "catalog_preferences_dao.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QVariant>

CatalogPreferencesDao::CatalogPreferencesDao(QSqlDatabase database)
    : m_database(database)
{
}

bool CatalogPreferencesDao::upsertPreference(const CatalogPreferenceRecord& preference)
{
    // Check if preference exists
    // Use empty string for catalog_id if it's empty
    QString catalogId = preference.catalogId.isEmpty() ? QString() : preference.catalogId;
    
    QSqlQuery query(m_database);
    query.prepare(R"(
        SELECT COUNT(*) FROM catalog_preferences
        WHERE addon_id = ? AND catalog_type = ? AND catalog_id = ?
    )");
    query.addBindValue(preference.addonId);
    query.addBindValue(preference.catalogType);
    query.addBindValue(catalogId);
    
    if (!query.exec() || !query.next()) {
        qWarning() << "Failed to check catalog preference:" << query.lastError().text();
        return false;
    }
    
    bool exists = query.value(0).toInt() > 0;
    
    if (exists) {
        // Update existing preference
        query.prepare(R"(
            UPDATE catalog_preferences SET
                enabled = ?, is_hero_source = ?, updated_at = ?
            WHERE addon_id = ? AND catalog_type = ? AND catalog_id = ?
        )");
        query.addBindValue(preference.enabled ? 1 : 0);
        query.addBindValue(preference.isHeroSource ? 1 : 0);
        query.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
        query.addBindValue(preference.addonId);
        query.addBindValue(preference.catalogType);
        query.addBindValue(catalogId);
    } else {
        // Insert new preference
        query.prepare(R"(
            INSERT INTO catalog_preferences (
                addon_id, catalog_type, catalog_id, enabled, is_hero_source, created_at, updated_at
            ) VALUES (?, ?, ?, ?, ?, ?, ?)
        )");
        query.addBindValue(preference.addonId);
        query.addBindValue(preference.catalogType);
        query.addBindValue(catalogId);
        query.addBindValue(preference.enabled ? 1 : 0);
        query.addBindValue(preference.isHeroSource ? 1 : 0);
        QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        query.addBindValue(now);
        query.addBindValue(now);
    }
    
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
    QString id = catalogId.isEmpty() ? QString() : catalogId;
    
    query.prepare(R"(
        SELECT * FROM catalog_preferences
        WHERE addon_id = ? AND catalog_type = ? AND catalog_id = ?
    )");
    query.addBindValue(addonId);
    query.addBindValue(catalogType);
    query.addBindValue(id);
    
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
    QString id = catalogId.isEmpty() ? QString() : catalogId;
    
    query.prepare(R"(
        UPDATE catalog_preferences SET enabled = ?, updated_at = ?
        WHERE addon_id = ? AND catalog_type = ? AND catalog_id = ?
    )");
    query.addBindValue(enabled ? 1 : 0);
    query.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    query.addBindValue(addonId);
    query.addBindValue(catalogType);
    query.addBindValue(id);
    
    if (!query.exec()) {
        qWarning() << "Failed to toggle catalog enabled:" << query.lastError().text();
        return false;
    }
    
    // If no rows were affected, create a new preference
    if (query.numRowsAffected() == 0) {
        CatalogPreferenceRecord preference;
        preference.addonId = addonId;
        preference.catalogType = catalogType;
        preference.catalogId = catalogId;
        preference.enabled = enabled;
        preference.isHeroSource = false;
        preference.createdAt = QDateTime::currentDateTimeUtc();
        preference.updatedAt = QDateTime::currentDateTimeUtc();
        return upsertPreference(preference);
    }
    
    return true;
}

bool CatalogPreferencesDao::setHeroCatalog(
    const QString& addonId,
    const QString& catalogType,
    const QString& catalogId)
{
    // First, unset all other hero catalogs (optional - you might want multiple hero catalogs)
    // For now, we'll allow multiple hero catalogs like the Dart version
    
    // Set this catalog as hero
    QSqlQuery query(m_database);
    QString id = catalogId.isEmpty() ? QString() : catalogId;
    
    query.prepare(R"(
        UPDATE catalog_preferences SET is_hero_source = 1, updated_at = ?
        WHERE addon_id = ? AND catalog_type = ? AND catalog_id = ?
    )");
    query.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    query.addBindValue(addonId);
    query.addBindValue(catalogType);
    query.addBindValue(id);
    
    if (!query.exec()) {
        qWarning() << "Failed to set hero catalog:" << query.lastError().text();
        return false;
    }
    
    // If no rows were affected, create a new preference with hero flag
    if (query.numRowsAffected() == 0) {
        CatalogPreferenceRecord preference;
        preference.addonId = addonId;
        preference.catalogType = catalogType;
        preference.catalogId = catalogId;
        preference.enabled = true;
        preference.isHeroSource = true;
        preference.createdAt = QDateTime::currentDateTimeUtc();
        preference.updatedAt = QDateTime::currentDateTimeUtc();
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
    QString id = catalogId.isEmpty() ? QString() : catalogId;
    
    query.prepare(R"(
        UPDATE catalog_preferences SET is_hero_source = 0, updated_at = ?
        WHERE addon_id = ? AND catalog_type = ? AND catalog_id = ?
    )");
    query.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    query.addBindValue(addonId);
    query.addBindValue(catalogType);
    query.addBindValue(id);
    
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

std::unique_ptr<CatalogPreferenceRecord> CatalogPreferencesDao::getHeroCatalog()
{
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM catalog_preferences WHERE is_hero_source = 1 LIMIT 1");
    
    if (!query.exec()) {
        qWarning() << "Failed to get hero catalog:" << query.lastError().text();
        return nullptr;
    }
    
    if (!query.next()) {
        return nullptr;
    }
    
    return std::make_unique<CatalogPreferenceRecord>(recordFromQuery(query));
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

