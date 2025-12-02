#ifndef ADDON_DAO_H
#define ADDON_DAO_H

#include <QSqlDatabase>
#include <QString>
#include <QDateTime>
#include <QList>
#include <memory>

struct AddonRecord
{
    QString id;
    QString name;
    QString version;
    QString description;
    QString manifestUrl;
    QString baseUrl;
    bool enabled;
    QString manifestData;
    QString resources;  // JSON string
    QString types;      // JSON string
    QDateTime createdAt;
    QDateTime updatedAt;
};

class AddonDao
{
public:
    explicit AddonDao(QSqlDatabase database);
    
    bool insertAddon(const AddonRecord& addon);
    bool updateAddon(const AddonRecord& addon);
    std::unique_ptr<AddonRecord> getAddonById(const QString& id);
    QList<AddonRecord> getAllAddons();
    QList<AddonRecord> getEnabledAddons();
    bool deleteAddon(const QString& id);
    bool toggleAddonEnabled(const QString& id, bool enabled);
    
private:
    QSqlDatabase m_database;
    
    AddonRecord recordFromQuery(const QSqlQuery& query);
};

#endif // ADDON_DAO_H





