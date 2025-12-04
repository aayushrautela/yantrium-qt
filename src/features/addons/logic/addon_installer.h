#ifndef ADDON_INSTALLER_H
#define ADDON_INSTALLER_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <functional>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// FIX: We must include the full header because we hold m_existingAddon by value
#include "../models/addon_config.h" 
#include "../models/addon_manifest.h"

class AddonInstaller : public QObject
{
    Q_OBJECT

public:
    using SuccessCallback = std::function<void(const AddonConfig&)>;
    using ErrorCallback = std::function<void(const QString&)>;

    explicit AddonInstaller(QObject* parent = nullptr);
    ~AddonInstaller();

    // Static entry point for installation
    static void installAddon(const QString& manifestUrl, 
                             SuccessCallback onSuccess, 
                             ErrorCallback onError = nullptr);

    // Static entry point for updates
    static void updateAddon(const AddonConfig& existingAddon, 
                            SuccessCallback onSuccess, 
                            ErrorCallback onError = nullptr);

signals:
    void addonInstalled(const AddonConfig& addon);
    void addonUpdated(const AddonConfig& addon);
    void error(const QString& errorMessage);

private slots:
    void onManifestFetched(const AddonManifest& manifest);
    void onManifestError(const QString& error);

private:
    void fetch(const QString& url);
    void processManifest(const AddonManifest& manifest);

    QString m_manifestUrl;
    AddonConfig m_existingAddon; 
    bool m_isUpdate;
};

#endif // ADDON_INSTALLER_H