#ifndef CATALOG_PREFERENCES_DAO_H
#define CATALOG_PREFERENCES_DAO_H

#include <QSqlDatabase>
#include <QString>
#include <QDateTime>
#include <QList>
#include <memory>

struct CatalogPreferenceRecord
{
    QString addonId;
    QString catalogType;
    QString catalogId;  // Can be empty/null
    bool enabled = true;         // Default to true
    bool isHeroSource = false;   // Default to false
    QDateTime createdAt;
    QDateTime updatedAt;
};

class CatalogPreferencesDao
{
public:
    explicit CatalogPreferencesDao();
    
    // Creates or completely overwrites a record
    bool upsertPreference(const CatalogPreferenceRecord& preference);

    std::unique_ptr<CatalogPreferenceRecord> getPreference(
        const QString& addonId,
        const QString& catalogType,
        const QString& catalogId = QString());

    QList<CatalogPreferenceRecord> getAllPreferences();

    // Toggles enabled state (creates record if missing)
    bool toggleCatalogEnabled(
        const QString& addonId,
        const QString& catalogType,
        const QString& catalogId,
        bool enabled);

    // Sets hero status to TRUE (creates record if missing).
    // Does NOT unset other hero catalogs (supports multiple sources).
    bool setHeroCatalog(
        const QString& addonId,
        const QString& catalogType,
        const QString& catalogId);

    // Sets hero status to FALSE
    bool unsetHeroCatalog(
        const QString& addonId,
        const QString& catalogType,
        const QString& catalogId);

    // Returns all catalogs marked as hero
    QList<CatalogPreferenceRecord> getHeroCatalogs();
    
private:
    QSqlDatabase m_database;
    
    CatalogPreferenceRecord recordFromQuery(const QSqlQuery& query);
    QString normalizeId(const QString& id) const;
};

#endif // CATALOG_PREFERENCES_DAO_H