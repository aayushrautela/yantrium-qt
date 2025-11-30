#ifndef CATALOG_PREFERENCES_SERVICE_H
#define CATALOG_PREFERENCES_SERVICE_H

#include <QObject>
#include <QString>
#include <QVariantList>
#include "core/database/catalog_preferences_dao.h"
#include "features/addons/logic/addon_repository.h"

class CatalogPreferencesService : public QObject
{
    Q_OBJECT

public:
    explicit CatalogPreferencesService(QObject* parent = nullptr);
    
    // Get all available catalogs with their preferences
    Q_INVOKABLE QVariantList getAvailableCatalogs();
    
    // Check if a catalog is enabled
    Q_INVOKABLE bool isCatalogEnabled(const QString& addonId, const QString& catalogType, const QString& catalogId = QString());
    
    // Toggle catalog enabled state
    Q_INVOKABLE bool toggleCatalogEnabled(const QString& addonId, const QString& catalogType, const QString& catalogId, bool enabled);
    
    // Set/unset hero catalog
    Q_INVOKABLE bool setHeroCatalog(const QString& addonId, const QString& catalogType, const QString& catalogId = QString());
    Q_INVOKABLE bool unsetHeroCatalog(const QString& addonId, const QString& catalogType, const QString& catalogId = QString());
    
    // Get hero catalogs
    Q_INVOKABLE QVariantList getHeroCatalogs();
    
    // Check if a catalog is a hero source
    Q_INVOKABLE bool isHeroSource(const QString& addonId, const QString& catalogType, const QString& catalogId = QString());

signals:
    void catalogsUpdated();
    void error(const QString& message);

private:
    CatalogPreferencesDao* m_dao;
    AddonRepository* m_addonRepository;
    
    QString capitalize(const QString& text);
};

#endif // CATALOG_PREFERENCES_SERVICE_H

