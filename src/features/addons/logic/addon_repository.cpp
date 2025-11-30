#include "addon_repository.h"
#include "addon_installer.h"
#include "addon_client.h"
#include <QJsonDocument>
#include <QVariantMap>
#include <QDebug>

AddonRepository::AddonRepository(QObject* parent)
    : QObject(parent)
{
    DatabaseManager& dbManager = DatabaseManager::instance();
    if (!dbManager.isInitialized()) {
        dbManager.initialize();
    }
    m_dao = new AddonDao(dbManager.database());
}

void AddonRepository::installAddon(const QString& manifestUrl)
{
    qDebug() << "[AddonRepository] Installing addon from:" << manifestUrl;
    
    // Create installer and connect signals
    AddonInstaller* installer = new AddonInstaller(this);
    installer->setManifestUrl(manifestUrl);
    installer->setIsUpdate(false);
    
    connect(installer, SIGNAL(addonInstalled(AddonConfig)), this, SLOT(onAddonInstalled(AddonConfig)));
    connect(installer, SIGNAL(error(QString)), this, SLOT(onInstallerError(QString)));
    
    // Create client and fetch manifest
    QString baseUrl = AddonClient::extractBaseUrl(manifestUrl);
    AddonClient* client = new AddonClient(baseUrl, installer);
    connect(client, SIGNAL(manifestFetched(AddonManifest)), installer, SLOT(onManifestFetched(AddonManifest)));
    connect(client, SIGNAL(error(QString)), installer, SLOT(onManifestError(QString)));
    
    qDebug() << "[AddonRepository] Fetching manifest from:" << baseUrl;
    client->fetchManifest();
}

QList<AddonConfig> AddonRepository::listAddons()
{
    QList<AddonRecord> records = m_dao->getAllAddons();
    QList<AddonConfig> addons;
    
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
    
    // Create installer and connect signals
    AddonInstaller* installer = new AddonInstaller(this);
    installer->setManifestUrl(existing.manifestUrl());
    installer->setExistingAddon(existing);
    installer->setIsUpdate(true);
    
    connect(installer, SIGNAL(addonUpdated(AddonConfig)), this, SLOT(onAddonUpdated(AddonConfig)));
    connect(installer, SIGNAL(error(QString)), this, SLOT(onInstallerError(QString)));
    
    // Create client and fetch manifest
    QString baseUrl = AddonClient::extractBaseUrl(existing.manifestUrl());
    AddonClient* client = new AddonClient(baseUrl, installer);
    connect(client, SIGNAL(manifestFetched(AddonManifest)), installer, SLOT(onManifestFetched(AddonManifest)));
    connect(client, SIGNAL(error(QString)), installer, SLOT(onManifestError(QString)));
    
    client->fetchManifest();
}

QList<AddonConfig> AddonRepository::getEnabledAddons()
{
    QList<AddonRecord> records = m_dao->getEnabledAddons();
    QList<AddonConfig> addons;
    
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
}

void AddonRepository::onAddonUpdated(const AddonConfig& addon)
{
    saveAddonToDatabase(addon);
    emit addonUpdated(addon);
}

void AddonRepository::onInstallerError(const QString& error)
{
    emit this->error(error);
}

int AddonRepository::listAddonsCount()
{
    return listAddons().size();
}

QVariantList AddonRepository::getAllAddons()
{
    QList<AddonConfig> addons = listAddons();
    QVariantList result;
    
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

QString AddonRepository::getAddonJson(const QString& id)
{
    AddonConfig addon = getAddon(id);
    if (addon.id().isEmpty()) {
        return QString();
    }
    
    // Return JSON representation (simplified for QML)
    QJsonObject obj;
    obj["id"] = addon.id();
    obj["name"] = addon.name();
    obj["version"] = addon.version();
    obj["description"] = addon.description();
    obj["enabled"] = addon.enabled();
    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
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
        // Update existing addon
        bool success = m_dao->updateAddon(record);
        if (!success) {
            qWarning() << "[AddonRepository] Failed to update addon in database";
            emit error("Failed to update addon in database");
        }
    } else {
        qDebug() << "[AddonRepository] Inserting new addon";
        // Insert new addon
        bool success = m_dao->insertAddon(record);
        if (!success) {
            qWarning() << "[AddonRepository] Failed to insert addon into database";
            emit error("Failed to insert addon into database");
        } else {
            qDebug() << "[AddonRepository] Addon successfully inserted";
        }
    }
}

