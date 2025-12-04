#include "addon_repository.h"
#include "addon_installer.h"
#include "addon_client.h"
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
    // MODERNIZATION: use make_unique
    m_dao = std::make_unique<AddonDao>(dbManager.database());
}

AddonRepository::~AddonRepository()
{
    // unique_ptr handles m_dao deletion automatically
}

void AddonRepository::installAddon(const QString& manifestUrl)
{
    qDebug() << "[AddonRepository] Installing addon from:" << manifestUrl;

    // Create installer
    // Note: 'this' parent means installer lives as long as Repository.
    // Ideally, AddonInstaller should delete itself (deleteLater) when finished.
    AddonInstaller* installer = new AddonInstaller(this);
    installer->setManifestUrl(manifestUrl);
    installer->setIsUpdate(false);

    // MODERNIZATION: New Signal/Slot Syntax
    connect(installer, &AddonInstaller::addonInstalled, 
            this, &AddonRepository::onAddonInstalled);
    connect(installer, &AddonInstaller::error, 
            this, &AddonRepository::onInstallerError);

    // Cleanup helper: Ensure installer deletes itself if logical flow allows
    // connect(installer, &AddonInstaller::finished, installer, &AddonInstaller::deleteLater);

    // Create client and fetch manifest
    QString baseUrl = AddonClient::extractBaseUrl(manifestUrl);
    
    // Parented to installer, so it dies when installer dies
    AddonClient* client = new AddonClient(baseUrl, installer); 
    
    // MODERNIZATION: New Signal/Slot Syntax
    connect(client, &AddonClient::manifestFetched, 
            installer, &AddonInstaller::onManifestFetched);
    connect(client, &AddonClient::error, 
            installer, &AddonInstaller::onManifestError);

    qDebug() << "[AddonRepository] Fetching manifest from:" << baseUrl;
    client->fetchManifest();
}

QList<AddonConfig> AddonRepository::listAddons()
{
    QList<AddonRecord> records = m_dao->getAllAddons();
    QList<AddonConfig> addons;
    addons.reserve(records.size()); // Optimization

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

void AddonRepository::updateAddon(const QString& id)
{
    AddonConfig existing = getAddon(id);
    if (existing.id().isEmpty()) {
        emit error(QString("Addon not found: %1").arg(id));
        return;
    }

    qDebug() << "[AddonRepository] Updating addon:" << existing.name();

    AddonInstaller* installer = new AddonInstaller(this);
    installer->setManifestUrl(existing.manifestUrl());
    installer->setExistingAddon(existing);
    installer->setIsUpdate(true);

    // MODERNIZATION: New Signal/Slot Syntax
    connect(installer, &AddonInstaller::addonUpdated, 
            this, &AddonRepository::onAddonUpdated);
    connect(installer, &AddonInstaller::error, 
            this, &AddonRepository::onInstallerError);

    QString baseUrl = AddonClient::extractBaseUrl(existing.manifestUrl());
    AddonClient* client = new AddonClient(baseUrl, installer);
    
    connect(client, &AddonClient::manifestFetched, 
            installer, &AddonInstaller::onManifestFetched);
    connect(client, &AddonClient::error, 
            installer, &AddonInstaller::onManifestError);

    client->fetchManifest();
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
    QJsonDocument doc = QJsonDocument::fromJson(addon.manifestData().toUtf8());
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
    qDebug() << "[AddonRepository] onAddonInstalled called for:" << addon.name() << addon.id();
    saveAddonToDatabase(addon);
    qDebug() << "[AddonRepository] Addon saved to database";
    emit addonInstalled(addon);
    
    // Optional: Clean up the installer sender
    // sender()->deleteLater();
}

void AddonRepository::onAddonUpdated(const AddonConfig& addon)
{
    saveAddonToDatabase(addon);
    emit addonUpdated(addon);
    
    // Optional: Clean up the installer sender
    // sender()->deleteLater();
}

void AddonRepository::onInstallerError(const QString& errorMsg)
{
    emit error(errorMsg);
    // Optional: Clean up the installer sender
    // sender()->deleteLater();
}

int AddonRepository::listAddonsCount()
{
    // Note: This is slightly inefficient as it queries full records just to get count.
    // In a real optimized scenario, DAO should have a countAllAddons() method.
    return listAddons().size();
}

QVariantList AddonRepository::getAllAddons()
{
    QList<AddonConfig> addons = listAddons();
    QVariantList result;
    result.reserve(addons.size());

    for (const AddonConfig& addon : addons) {
        QVariantMap map;
        map["id"] = addon.id();
        map["name"] = addon.name();
        map["version"] = addon.version();
        map["enabled"] = addon.enabled();
        map["manifestUrl"] = addon.manifestUrl();
        result.append(map);
    }

    return result;
}

// MODERNIZATION: Renamed from getAddonJson
// Returns QVariantMap. In QML, you use this exactly like a JS object:
// var addon = repo.getAddonDetails("123");
// console.log(addon.name);
QVariantMap AddonRepository::getAddonDetails(const QString& id)
{
    AddonConfig addon = getAddon(id);
    QVariantMap map;

    if (addon.id().isEmpty()) {
        return map;
    }

    map["id"] = addon.id();
    map["name"] = addon.name();
    map["version"] = addon.version();
    map["description"] = addon.description();
    map["enabled"] = addon.enabled();
    
    return map;
}

int AddonRepository::getEnabledAddonsCount()
{
    return getEnabledAddons().size();
}

void AddonRepository::saveAddonToDatabase(const AddonConfig& addon)
{
    qDebug() << "[AddonRepository] Saving addon to database:" << addon.id();
    AddonRecord record = addon.toDatabaseRecord();

    // Check if addon already exists
    std::unique_ptr<AddonRecord> existing = m_dao->getAddonById(addon.id());
    
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