#ifndef ADDON_REPOSITORY_H
#define ADDON_REPOSITORY_H

#include <QObject>
#include <QList>
#include <QString>
#include <QJsonArray>
#include <QVariantList>
#include <QVariantMap>
#include <memory> // Required for std::unique_ptr
#include <QtQmlIntegration/qqmlintegration.h>

#include "../models/addon_config.h"
#include "../models/addon_manifest.h"
#include "core/database/addon_dao.h"
#include "core/database/database_manager.h"

// Forward declaration to reduce compile time dependencies if possible
class AddonInstaller; 
class AddonClient;

class AddonRepository : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit AddonRepository(QObject* parent = nullptr);
    ~AddonRepository(); // Added destructor for unique_ptr cleanup

    // Install addon from manifest URL
    Q_INVOKABLE void installAddon(const QString& manifestUrl);

    // List all installed addons (returns count)
    Q_INVOKABLE int listAddonsCount();

    // Get specific addon details. 
    // MODERNIZATION: Returns QVariantMap instead of JSON String. 
    // QML can read this natively as a JS Object.
    Q_INVOKABLE QVariantMap getAddonDetails(const QString& id);

    // Get all addons as QVariantList for QML
    Q_INVOKABLE QVariantList getAllAddons();

    // Enable/disable addon
    Q_INVOKABLE bool enableAddon(const QString& id);
    Q_INVOKABLE bool disableAddon(const QString& id);

    // Remove addon
    Q_INVOKABLE bool removeAddon(const QString& id);

    // Update addon manifest (refresh from URL)
    Q_INVOKABLE void updateAddon(const QString& id);

    // Get only enabled addons count
    Q_INVOKABLE int getEnabledAddonsCount();

    // C++ methods (not exposed to QML directly)
    QList<AddonConfig> listAddons();
    AddonConfig getAddon(const QString& id);
    QList<AddonConfig> getEnabledAddons();

    // Get manifest from addon config
    AddonManifest getManifest(const AddonConfig& addon);

    // Static helper: Check if addon supports a resource
    static bool hasResource(const QJsonArray& resources, const QString& resourceName);

signals:
    void addonInstalled(const AddonConfig& addon);
    void addonUpdated(const AddonConfig& addon);
    void addonRemoved(const QString& id);
    void error(const QString& errorMessage);

private slots:
    void onAddonInstalled(const AddonConfig& addon);
    void onAddonUpdated(const AddonConfig& addon);
    void onInstallerError(const QString& error);

private:
    // MODERNIZATION: Smart pointer handles memory automatically
    std::unique_ptr<AddonDao> m_dao;

    void saveAddonToDatabase(const AddonConfig& addon);
};

#endif // ADDON_REPOSITORY_H