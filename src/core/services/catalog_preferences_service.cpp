#include "catalog_preferences_service.h"
#include "logging_service.h"
#include "logging_service.h"
#include "core/database/database_manager.h"
#include "features/addons/models/addon_manifest.h"

CatalogPreferencesService::CatalogPreferencesService(
    std::unique_ptr<CatalogPreferencesDao> dao,
    std::shared_ptr<AddonRepository> addonRepository,
    QObject* parent)
    : QObject(parent)
    , m_dao(std::move(dao))
    , m_addonRepository(std::move(addonRepository))
{
}

QVariantList CatalogPreferencesService::getAvailableCatalogs()
{
    QVariantList catalogs;
    
    try {
        // Get all enabled addons
        QList<AddonConfig> enabledAddons = m_addonRepository->getEnabledAddons();
        
        for (const AddonConfig& addon : enabledAddons) {
            // Check if addon has catalog resource
            if (!AddonRepository::hasResource(addon.resources, "catalog")) {
                continue;
            }
            
            // Get manifest
            AddonManifest manifest = m_addonRepository->getManifest(addon);
            
            // Process each catalog definition
            for (const CatalogDefinition& catalogDef : manifest.catalogs()) {
                QString catalogId = catalogDef.id().isEmpty() ? QString() : catalogDef.id();
                
                // Skip search catalogs - they are only used for search functionality, not catalog management
                if (catalogId == "search") {
                    continue;
                }
                
                // Get preference (or default to enabled)
                auto preference = m_dao->getPreference(addon.id, catalogDef.type(), catalogId);
                bool enabled = preference ? preference->enabled : true;
                bool isHeroSource = preference ? preference->isHeroSource : false;
                int order = preference ? preference->order : 0;
                
                // Build catalog name
                QString catalogName = catalogDef.name();
                if (catalogName.isEmpty()) {
                    catalogName = capitalize(catalogDef.type());
                    if (!catalogId.isEmpty()) {
                        catalogName += " - " + catalogId;
                    }
                }
                
                QVariantMap catalogInfo;
                catalogInfo["addonId"] = addon.id;
                catalogInfo["addonName"] = addon.name;
                catalogInfo["catalogType"] = catalogDef.type();
                catalogInfo["catalogId"] = catalogId;
                catalogInfo["catalogName"] = catalogName;
                catalogInfo["enabled"] = enabled;
                catalogInfo["isHeroSource"] = isHeroSource;
                catalogInfo["order"] = order;
                catalogInfo["uniqueId"] = QString("%1|%2|%3").arg(addon.id, catalogDef.type(), catalogId);
                
                catalogs.append(catalogInfo);
            }
        }
    } catch (const std::exception& e) {
        LoggingService::logWarning("CatalogPreferencesService", QString("Error getting available catalogs: %1").arg(e.what()));
        QString errorMsg = QString("Failed to get catalogs: %1").arg(e.what());
        LoggingService::report(errorMsg, "DATABASE_ERROR", "CatalogPreferencesService");
        emit error(errorMsg);
    }
    
    return catalogs;
}

bool CatalogPreferencesService::isCatalogEnabled(
    const QString& addonId,
    const QString& catalogType,
    const QString& catalogId)
{
    auto preference = m_dao->getPreference(addonId, catalogType, catalogId);
    // Default to enabled if no preference exists
    return preference ? preference->enabled : true;
}

bool CatalogPreferencesService::toggleCatalogEnabled(
    const QString& addonId,
    const QString& catalogType,
    const QString& catalogId,
    bool enabled)
{
    bool success = m_dao->toggleCatalogEnabled(addonId, catalogType, catalogId, enabled);
    if (success) {
        emit catalogsUpdated();
    } else {
        emit error("Failed to toggle catalog enabled state");
    }
    return success;
}

bool CatalogPreferencesService::setHeroCatalog(
    const QString& addonId,
    const QString& catalogType,
    const QString& catalogId)
{
    bool success = m_dao->setHeroCatalog(addonId, catalogType, catalogId);
    if (success) {
        emit catalogsUpdated();
    } else {
        emit error("Failed to set hero catalog");
    }
    return success;
}

bool CatalogPreferencesService::unsetHeroCatalog(
    const QString& addonId,
    const QString& catalogType,
    const QString& catalogId)
{
    bool success = m_dao->unsetHeroCatalog(addonId, catalogType, catalogId);
    if (success) {
        emit catalogsUpdated();
    } else {
        emit error("Failed to unset hero catalog");
    }
    return success;
}

QVariantList CatalogPreferencesService::getHeroCatalogs()
{
    QVariantList heroCatalogs;
    
    QList<CatalogPreferenceRecord> heroPreferences = m_dao->getHeroCatalogs();
    
    for (const CatalogPreferenceRecord& preference : heroPreferences) {
        try {
            AddonConfig addon = m_addonRepository->getAddon(preference.addonId);
            AddonManifest manifest = m_addonRepository->getManifest(addon);
            
            // Find the catalog definition
            CatalogDefinition catalogDef;
            for (const CatalogDefinition& def : manifest.catalogs()) {
                QString defId = def.id().isEmpty() ? QString() : def.id();
                if (def.type() == preference.catalogType && defId == preference.catalogId) {
                    catalogDef = def;
                    break;
                }
            }
            
            QString catalogName = catalogDef.name();
            if (catalogName.isEmpty()) {
                catalogName = capitalize(preference.catalogType);
                if (!preference.catalogId.isEmpty()) {
                    catalogName += " - " + preference.catalogId;
                }
            }
            
            QVariantMap catalogInfo;
            catalogInfo["addonId"] = preference.addonId;
            catalogInfo["addonName"] = addon.name;
            catalogInfo["catalogType"] = preference.catalogType;
            catalogInfo["catalogId"] = preference.catalogId;
            catalogInfo["catalogName"] = catalogName;
            catalogInfo["enabled"] = preference.enabled;
            catalogInfo["isHeroSource"] = true;
            
            heroCatalogs.append(catalogInfo);
        } catch (...) {
            // Skip if addon not found or other error
            continue;
        }
    }
    
    return heroCatalogs;
}

bool CatalogPreferencesService::isHeroSource(
    const QString& addonId,
    const QString& catalogType,
    const QString& catalogId)
{
    auto preference = m_dao->getPreference(addonId, catalogType, catalogId);
    return preference ? preference->isHeroSource : false;
}

bool CatalogPreferencesService::updateCatalogOrder(const QVariantList& catalogOrder)
{
    bool success = m_dao->updateCatalogOrder(catalogOrder);
    if (success) {
        emit catalogsUpdated();
    } else {
        emit error("Failed to update catalog order");
    }
    return success;
}

QString CatalogPreferencesService::capitalize(const QString& text)
{
    if (text.isEmpty()) return text;
    return text.left(1).toUpper() + text.mid(1);
}

