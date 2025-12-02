#ifndef ADDON_MANIFEST_H
#define ADDON_MANIFEST_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <QStringList>
#include <QMap>
#include "catalog_definition.h"

class AddonManifest
{
public:
    AddonManifest();
    AddonManifest(const QString& id,
                  const QString& version,
                  const QString& name,
                  const QString& description = QString(),
                  const QJsonArray& resources = QJsonArray(),
                  const QStringList& types = QStringList(),
                  const QList<CatalogDefinition>& catalogs = QList<CatalogDefinition>());
    
    static AddonManifest fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    static bool validate(const AddonManifest& manifest);
    
    QString id() const { return m_id; }
    void setId(const QString& id) { m_id = id; }
    
    QString version() const { return m_version; }
    void setVersion(const QString& version) { m_version = version; }
    
    QString name() const { return m_name; }
    void setName(const QString& name) { m_name = name; }
    
    QString description() const { return m_description; }
    void setDescription(const QString& description) { m_description = description; }
    
    QJsonArray resources() const { return m_resources; }
    void setResources(const QJsonArray& resources) { m_resources = resources; }
    
    QStringList types() const { return m_types; }
    void setTypes(const QStringList& types) { m_types = types; }
    
    QList<CatalogDefinition> catalogs() const { return m_catalogs; }
    void setCatalogs(const QList<CatalogDefinition>& catalogs) { m_catalogs = catalogs; }
    
    QStringList idPrefixes() const { return m_idPrefixes; }
    void setIdPrefixes(const QStringList& idPrefixes) { m_idPrefixes = idPrefixes; }
    
    QString background() const { return m_background; }
    void setBackground(const QString& background) { m_background = background; }
    
    QString logo() const { return m_logo; }
    void setLogo(const QString& logo) { m_logo = logo; }
    
    QString contactEmail() const { return m_contactEmail; }
    void setContactEmail(const QString& contactEmail) { m_contactEmail = contactEmail; }
    
    QJsonObject behaviorHints() const { return m_behaviorHints; }
    void setBehaviorHints(const QJsonObject& behaviorHints) { m_behaviorHints = behaviorHints; }
    
private:
    QString m_id;
    QString m_version;
    QString m_name;
    QString m_description;
    QJsonArray m_resources;
    QStringList m_types;
    QList<CatalogDefinition> m_catalogs;
    QStringList m_idPrefixes;
    QString m_background;
    QString m_logo;
    QString m_contactEmail;
    QJsonObject m_behaviorHints;
};

#endif // ADDON_MANIFEST_H




