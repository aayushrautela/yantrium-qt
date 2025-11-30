#include "addon_installer.h"
#include "addon_client.h"
#include <QJsonDocument>
#include <QDebug>

AddonInstaller::AddonInstaller(QObject* parent)
    : QObject(parent)
    , m_isUpdate(false)
{
}

void AddonInstaller::installAddon(const QString& manifestUrl, QObject* receiver, const char* slot)
{
    AddonInstaller* installer = new AddonInstaller(receiver);
    installer->m_manifestUrl = manifestUrl;
    installer->m_isUpdate = false;
    
    QObject::connect(installer, SIGNAL(addonInstalled(AddonConfig)), receiver, slot);
    QObject::connect(installer, SIGNAL(error(QString)), installer, SLOT(deleteLater()));
    
    AddonClient* client = new AddonClient(AddonClient::extractBaseUrl(manifestUrl), installer);
    QObject::connect(client, SIGNAL(manifestFetched(AddonManifest)), installer, SLOT(onManifestFetched(AddonManifest)));
    QObject::connect(client, SIGNAL(error(QString)), installer, SLOT(onManifestError(QString)));
    client->fetchManifest();
}

void AddonInstaller::updateAddon(const AddonConfig& existingAddon, QObject* receiver, const char* slot)
{
    AddonInstaller* installer = new AddonInstaller(receiver);
    installer->m_manifestUrl = existingAddon.manifestUrl();
    installer->m_existingAddon = existingAddon;
    installer->m_isUpdate = true;
    
    QObject::connect(installer, SIGNAL(addonUpdated(AddonConfig)), receiver, slot);
    QObject::connect(installer, SIGNAL(error(QString)), installer, SLOT(deleteLater()));
    
    AddonClient* client = new AddonClient(AddonClient::extractBaseUrl(existingAddon.manifestUrl()), installer);
    QObject::connect(client, SIGNAL(manifestFetched(AddonManifest)), installer, SLOT(onManifestFetched(AddonManifest)));
    QObject::connect(client, SIGNAL(error(QString)), installer, SLOT(onManifestError(QString)));
    client->fetchManifest();
}

void AddonInstaller::onManifestFetched(const AddonManifest& manifest)
{
    processManifest(manifest);
    deleteLater();
}

void AddonInstaller::onManifestError(const QString& error)
{
    emit this->error(error);
    deleteLater();
}

void AddonInstaller::processManifest(const AddonManifest& manifest)
{
    // Validate manifest
    if (!AddonClient::validateManifest(manifest)) {
        emit error("Invalid manifest: missing required fields");
        return;
    }
    
    // Extract base URL
    QString baseUrl = AddonClient::extractBaseUrl(m_manifestUrl);
    
    // Create AddonConfig
    QDateTime now = QDateTime::currentDateTime();
    QJsonDocument manifestDoc(manifest.toJson());
    QString manifestData = QString::fromUtf8(manifestDoc.toJson(QJsonDocument::Compact));
    
    AddonConfig addon;
    
    if (m_isUpdate) {
        // Update existing addon (preserve enabled state and creation date)
        addon = m_existingAddon;
        addon.setName(manifest.name());
        addon.setVersion(manifest.version());
        addon.setDescription(manifest.description());
        addon.setManifestData(manifestData);
        addon.setResources(manifest.resources());
        
        QStringList types;
        for (const QString& type : manifest.types()) {
            types.append(type);
        }
        addon.setTypes(types);
        addon.setUpdatedAt(now);
        
        emit addonUpdated(addon);
    } else {
        // Create new addon
        QStringList types;
        for (const QString& type : manifest.types()) {
            types.append(type);
        }
        
        addon = AddonConfig(
            manifest.id(),
            manifest.name(),
            manifest.version(),
            manifest.description(),
            m_manifestUrl,
            baseUrl,
            true, // enabled by default
            manifestData,
            manifest.resources(),
            types,
            now,
            now
        );
        
        emit addonInstalled(addon);
    }
}

