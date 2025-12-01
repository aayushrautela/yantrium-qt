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
    bool enabled;
    bool isHeroSource;
    QDateTime createdAt;
    QDateTime updatedAt;
};

class CatalogPreferencesDao
{
public:
    explicit CatalogPreferencesDao(QSqlDatabase database);
    
    bool upsertPreference(const CatalogPreferenceRecord& preference);
    std::unique_ptr<CatalogPreferenceRecord> getPreference(
        const QString& addonId,
        const QString& catalogType,
        const QString& catalogId = QString());
    QList<CatalogPreferenceRecord> getAllPreferences();
    bool toggleCatalogEnabled(
        const QString& addonId,
        const QString& catalogType,
        const QString& catalogId,
        bool enabled);
    bool setHeroCatalog(
        const QString& addonId,
        const QString& catalogType,
        const QString& catalogId);
    bool unsetHeroCatalog(
        const QString& addonId,
        const QString& catalogType,
        const QString& catalogId);
    QList<CatalogPreferenceRecord> getHeroCatalogs();
    std::unique_ptr<CatalogPreferenceRecord> getHeroCatalog();  // Single hero catalog (for backward compatibility)
    
private:
    QSqlDatabase m_database;
    
    CatalogPreferenceRecord recordFromQuery(const QSqlQuery& query);
};

#endif // CATALOG_PREFERENCES_DAO_H


