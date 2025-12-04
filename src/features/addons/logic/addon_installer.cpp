#include "addon_installer.h"
#include "addon_client.h"
#include "../models/addon_config.h"
#include "../models/addon_manifest.h"

#include <QDebug>

AddonInstaller::AddonInstaller(QObject* parent)
    : QObject(parent)
    , m_isUpdate(false)
{
}

AddonInstaller::~AddonInstaller()
{
}

void AddonInstaller::installAddon(const QString& manifestUrl, SuccessCallback onSuccess, ErrorCallback onError)
{
    AddonInstaller* installer = new AddonInstaller(nullptr);
    installer->m_manifestUrl = manifestUrl;
    installer->m_isUpdate = false;

    connect(installer, &AddonInstaller::addonInstalled, [installer, onSuccess](const AddonConfig& addon) {
        if (onSuccess) {
            onSuccess(addon);
        }
        installer->deleteLater();
    });

    connect(installer, &AddonInstaller::error, [installer, onError](const QString& msg) {
        if (onError) {
            onError(msg);
        } else {
            qWarning() << "Addon install error (unhandled):" << msg;
        }
        installer->deleteLater();
    });

    installer->fetch(manifestUrl);
}

void AddonInstaller::updateAddon(const AddonConfig& existingAddon, SuccessCallback onSuccess, ErrorCallback onError)
{
    AddonInstaller* installer = new AddonInstaller(nullptr);
    installer->m_existingAddon = existingAddon;
    // FIXED: Direct access
    installer->m_manifestUrl = existingAddon.manifestUrl;
    installer->m_isUpdate = true;

    connect(installer, &AddonInstaller::addonUpdated, [installer, onSuccess](const AddonConfig& addon) {
        if (onSuccess) {
            onSuccess(addon);
        }
        installer->deleteLater();
    });

    connect(installer, &AddonInstaller::error, [installer, onError](const QString& msg) {
        if (onError) {
            onError(msg);
        } else {
            qWarning() << "Addon update error (unhandled):" << msg;
        }
        installer->deleteLater();
    });

    // FIXED: Direct access
    installer->fetch(existingAddon.manifestUrl);
}

void AddonInstaller::fetch(const QString& url)
{
    QString baseUrl = AddonClient::extractBaseUrl(url);
    
    AddonClient* client = new AddonClient(baseUrl, this);

    connect(client, &AddonClient::manifestFetched, this, &AddonInstaller::onManifestFetched);
    connect(client, &AddonClient::error, this, &AddonInstaller::onManifestError);

    client->fetchManifest();
}

void AddonInstaller::onManifestFetched(const AddonManifest& manifest)
{
    processManifest(manifest);
}

void AddonInstaller::onManifestError(const QString& errorMsg)
{
    emit error(errorMsg);
}

void AddonInstaller::processManifest(const AddonManifest& manifest)
{
    if (!AddonClient::validateManifest(manifest)) {
        emit error("Invalid manifest: missing required fields");
        return;
    }

    QString baseUrl = AddonClient::extractBaseUrl(m_manifestUrl);

    QJsonDocument manifestDoc(manifest.toJson());
    QString manifestData = QString::fromUtf8(manifestDoc.toJson(QJsonDocument::Compact));
    QDateTime now = QDateTime::currentDateTime();

    // FIXED: Parse types manually for QStringList
    QStringList types;
    for (const auto& type : manifest.types()) {
        types.append(type);
    }

    AddonConfig addon;

    if (m_isUpdate) {
        // Update logic: Start with existing
        addon = m_existingAddon;
        
        // FIXED: Direct assignment (No setters)
        addon.name = manifest.name();
        addon.version = manifest.version();
        addon.description = manifest.description();
        addon.manifestData = manifestData;
        addon.resources = manifest.resources();
        addon.types = types;
        addon.updatedAt = now;

        emit addonUpdated(addon);
    } else {
        // Install logic: Create fresh config
        // FIXED: Using direct member assignment instead of the old constructor
        addon.id = manifest.id();
        addon.name = manifest.name();
        addon.version = manifest.version();
        addon.description = manifest.description();
        addon.manifestUrl = m_manifestUrl;
        addon.baseUrl = baseUrl;
        addon.enabled = true;
        addon.manifestData = manifestData;
        addon.resources = manifest.resources();
        addon.types = types;
        addon.createdAt = now;
        addon.updatedAt = now;

        emit addonInstalled(addon);
    }
}