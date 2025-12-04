#include "addon_repository.h"
#include "addon_installer.h"
// Note: We don't need "addon_client.h" here anymore because the Installer encapsulates it.
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantMap>
#include <QDebug>

AddonRepository::AddonRepository(QObject* parent)
    : QObject(parent)
{
    DatabaseManager& dbManager = DatabaseManager::instance();
    if (!dbManager.isInitialized()) {
        dbManager.initialize();
    }
    
    // Modern: use make_unique
    m_dao = std::make_unique<AddonDao>(dbManager.database());
}

AddonRepository::~AddonRepository()
{
    // unique_ptr handles m_dao deletion automatically
}

void AddonRepository::installAddon(const QString& manifestUrl)
{
    qDebug() << "[AddonRepository] Installing addon from:" << manifestUrl;

    AddonInstaller::installAddon(manifestUrl,
        // Success Callback
        [this](const AddonConfig& addon) {
            this->onAddonInstalled(addon);
        },
        // Error Callback
        [this](const QString& errorMsg) {
            this->onInstallerError(errorMsg);
        }
    );
}

void AddonRepository::updateAddon(const QString& id)
{
    AddonConfig existing = getAddon(id);
    // FIXED: Direct access .id instead of .id()
    if (existing.id.isEmpty()) {
        emit error(QString("Addon not found: %1").arg(id));
        return;
    }

    // FIXED: Direct access .name
    qDebug() << "[AddonRepository] Updating addon:" << existing.name;

    AddonInstaller::updateAddon(existing,
        // Success Callback
        [this](const AddonConfig& updatedAddon) {
            this->onAddonUpdated(updatedAddon);
        },
        // Error Callback
        [this](const QString& errorMsg) {
            this->onInstallerError(errorMsg);
        }
    );
}

QList<AddonConfig> AddonRepository::listAddons()
{
    QList<AddonRecord> records = m_dao->getAllAddons();
    QList<AddonConfig> addons;
    addons.reserve(records.size()); 

    for (const AddonRecord& record : records) {
        addons.append(AddonConfig::fromDatabase(record));
    }

    return addons;
}

AddonConfig AddonRepository::getAddon(const QString& id)
{
    std::unique_ptr<AddonRecord> record = m_dao->getAddonById(id);
    if (!record) {
        return AddonConfig(); // Return empty config if not found
    }
    return AddonConfig::fromDatabase(*record);
}

bool AddonRepository::enableAddon(const QString& id)
{
    return m_dao->toggleAddonEnabled(id, true);
}

bool AddonRepository::disableAddon(const QString& id)
{
    return m_dao->toggleAddonEnabled(id, false);
}

bool AddonRepository::removeAddon(const QString& id)
{
    bool success = m_dao->deleteAddon(id);
    if (success) {
        emit addonRemoved(id);
    }
    return success;
}

QList<AddonConfig> AddonRepository::getEnabledAddons()
{
    QList<AddonRecord> records = m_dao->getEnabledAddons();
    QList<AddonConfig> addons;
    addons.reserve(records.size());

    for (const AddonRecord& record : records) {
        addons.append(AddonConfig::fromDatabase(record));
    }

    return addons;
}

AddonManifest AddonRepository::getManifest(const AddonConfig& addon)
{
    // FIXED: Direct access .manifestData
    QJsonDocument doc = QJsonDocument::fromJson(addon.manifestData.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        return AddonManifest();
    }
    return AddonManifest::fromJson(doc.object());
}

bool AddonRepository::hasResource(const QJsonArray& resources, const QString& resourceName)
{
    for (const QJsonValue& resource : resources) {
        if (resource.isString()) {
            if (resource.toString() == resourceName) {
                return true;
            }
        } else if (resource.isObject()) {
            QJsonObject obj = resource.toObject();
            if (obj["name"].toString() == resourceName) {
                return true;
            }
        }
    }
    return false;
}

void AddonRepository::onAddonInstalled(const AddonConfig& addon)
{
    // FIXED: Direct access
    qDebug() << "[AddonRepository] onAddonInstalled called for:" << addon.name << addon.id;
    saveAddonToDatabase(addon);
    qDebug() << "[AddonRepository] Addon saved to database";
    emit addonInstalled(addon);
}

void AddonRepository::onAddonUpdated(const AddonConfig& addon)
{
    saveAddonToDatabase(addon);
    emit addonUpdated(addon);
}

void AddonRepository::onInstallerError(const QString& errorMsg)
{
    emit error(errorMsg);
}

int AddonRepository::listAddonsCount()
{
    return listAddons().size();
}

QVariantList AddonRepository::getAllAddons()
{
    QList<AddonConfig> addons = listAddons();
    QVariantList result;
    result.reserve(addons.size());

    for (const AddonConfig& addon : addons) {
        QVariantMap map;
        // FIXED: All direct access
        map["id"] = addon.id;
        map["name"] = addon.name;
        map["version"] = addon.version;
        map["enabled"] = addon.enabled;
        map["manifestUrl"] = addon.manifestUrl;
        result.append(map);
    }

    return result;
}

QVariantMap AddonRepository::getAddonDetails(const QString& id)
{
    AddonConfig addon = getAddon(id);
    QVariantMap map;

    // FIXED: Direct access
    if (addon.id.isEmpty()) {
        return map;
    }

    map["id"] = addon.id;
    map["name"] = addon.name;
    map["version"] = addon.version;
    map["description"] = addon.description;
    map["enabled"] = addon.enabled;
    
    return map;
}

int AddonRepository::getEnabledAddonsCount()
{
    return getEnabledAddons().size();
}

void AddonRepository::saveAddonToDatabase(const AddonConfig& addon)
{
    // FIXED: Direct access
    qDebug() << "[AddonRepository] Saving addon to database:" << addon.id;
    AddonRecord record = addon.toDatabaseRecord();

    // Check if addon already exists
    std::unique_ptr<AddonRecord> existing = m_dao->getAddonById(addon.id);
    
    if (existing) {
        qDebug() << "[AddonRepository] Updating existing addon";
        if (!m_dao->updateAddon(record)) {
            qWarning() << "[AddonRepository] Failed to update addon in database";
            emit error("Failed to update addon in database");
        }
    } else {
        qDebug() << "[AddonRepository] Inserting new addon";
        if (!m_dao->insertAddon(record)) {
            qWarning() << "[AddonRepository] Failed to insert addon into database";
            emit error("Failed to insert addon into database");
        } else {
            qDebug() << "[AddonRepository] Addon successfully inserted";
        }
    }
}