#ifndef ADDON_DAO_H
#define ADDON_DAO_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QDateTime>
#include <QList>
#include <memory>
#include <string_view>

#include "database_manager.h"

struct AddonRecord
{
    QString id;
    QString name;
    QString version;
    QString description;
    QString manifestUrl;
    QString baseUrl;
    bool enabled = false;
    QString manifestData;
    QString resources;  // JSON string
    QString types;      // JSON string
    QDateTime createdAt;
    QDateTime updatedAt;

    // Default constructor with modern initialization
    AddonRecord() = default;

    // Constructor for convenience
    AddonRecord(QString id, QString name, QString version, bool enabled = false)
        : id(std::move(id)), name(std::move(name)), version(std::move(version)), enabled(enabled) {}
};

class AddonDao
{
public:
    // Modern constructor - explicit and noexcept
    explicit AddonDao() noexcept;

    // Critical database operations - mark as [[nodiscard]]
    [[nodiscard]] bool insertAddon(const AddonRecord& addon);
    [[nodiscard]] bool updateAddon(const AddonRecord& addon);
    [[nodiscard]] std::unique_ptr<AddonRecord> getAddonById(std::string_view id);
    [[nodiscard]] QList<AddonRecord> getAllAddons();
    [[nodiscard]] QList<AddonRecord> getEnabledAddons();
    [[nodiscard]] bool deleteAddon(std::string_view id);
    [[nodiscard]] bool toggleAddonEnabled(std::string_view id, bool enabled);

private:
    // Helper method - const and noexcept where safe
    [[nodiscard]] AddonRecord recordFromQuery(const QSqlQuery& query) const noexcept;

    // Database connection getter - modern approach (replaces member variable)
    [[nodiscard]] QSqlDatabase getDatabase() const noexcept {
        return QSqlDatabase::database(DatabaseManager::CONNECTION_NAME);
    }
};

#endif // ADDON_DAO_H






