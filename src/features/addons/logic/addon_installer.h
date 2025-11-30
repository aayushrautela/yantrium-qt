#ifndef ADDON_INSTALLER_H
#define ADDON_INSTALLER_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include "../models/addon_config.h"
#include "../models/addon_manifest.h"

class AddonInstaller : public QObject
{
    Q_OBJECT

public:
    explicit AddonInstaller(QObject* parent = nullptr);
    
    // Install addon from manifest URL
    static void installAddon(const QString& manifestUrl, QObject* receiver, const char* slot);
    
    // Update addon manifest (refresh from URL)
    static void updateAddon(const AddonConfig& existingAddon, QObject* receiver, const char* slot);
    
    // Setter methods for direct installation
    void setManifestUrl(const QString& url) { m_manifestUrl = url; }
    void setExistingAddon(const AddonConfig& addon) { m_existingAddon = addon; }
    void setIsUpdate(bool isUpdate) { m_isUpdate = isUpdate; }
    
signals:
    void addonInstalled(const AddonConfig& addon);
    void addonUpdated(const AddonConfig& addon);
    void error(const QString& errorMessage);
    
private slots:
    void onManifestFetched(const AddonManifest& manifest);
    void onManifestError(const QString& error);
    
private:
    QString m_manifestUrl;
    AddonConfig m_existingAddon;
    bool m_isUpdate;
    
    void processManifest(const AddonManifest& manifest);
};

#endif // ADDON_INSTALLER_H

