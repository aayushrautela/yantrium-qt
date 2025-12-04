#ifndef ADDON_MANIFEST_H
#define ADDON_MANIFEST_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <QStringList>
#include "catalog_definition.h"

class AddonManifest
{
public:
    // Default constructor can be defaulted because we use in-class initialization below
    AddonManifest() = default;

    AddonManifest(const QString& id,
                  const QString& version,
                  const QString& name,
                  const QString& description = {},
                  const QJsonArray& resources = {},
                  const QStringList& types = {},
                  const QList<CatalogDefinition>& catalogs = {});
    
    // [[nodiscard]] warns if the return value is ignored
    [[nodiscard]] static AddonManifest fromJson(const QJsonObject& json);
    [[nodiscard]] QJsonObject toJson() const;
    [[nodiscard]] static bool validate(const AddonManifest& manifest);
    
    // Getters
    [[nodiscard]] QString id() const { return m_id; }
    [[nodiscard]] QString version() const { return m_version; }
    [[nodiscard]] QString name() const { return m_name; }
    [[nodiscard]] QString description() const { return m_description; }
    [[nodiscard]] QJsonArray resources() const { return m_resources; }
    [[nodiscard]] QStringList types() const { return m_types; }
    [[nodiscard]] QList<CatalogDefinition> catalogs() const { return m_catalogs; }
    [[nodiscard]] QStringList idPrefixes() const { return m_idPrefixes; }
    [[nodiscard]] QString background() const { return m_background; }
    [[nodiscard]] QString logo() const { return m_logo; }
    [[nodiscard]] QString contactEmail() const { return m_contactEmail; }
    [[nodiscard]] QJsonObject behaviorHints() const { return m_behaviorHints; }

    // Setters
    // In Qt, const QString& is preferred over std::move for setters due to Implicit Sharing (COW)
    void setId(const QString& id) { m_id = id; }
    void setVersion(const QString& version) { m_version = version; }
    void setName(const QString& name) { m_name = name; }
    void setDescription(const QString& description) { m_description = description; }
    void setResources(const QJsonArray& resources) { m_resources = resources; }
    void setTypes(const QStringList& types) { m_types = types; }
    void setCatalogs(const QList<CatalogDefinition>& catalogs) { m_catalogs = catalogs; }
    void setIdPrefixes(const QStringList& idPrefixes) { m_idPrefixes = idPrefixes; }
    void setBackground(const QString& background) { m_background = background; }
    void setLogo(const QString& logo) { m_logo = logo; }
    void setContactEmail(const QString& contactEmail) { m_contactEmail = contactEmail; }
    void setBehaviorHints(const QJsonObject& behaviorHints) { m_behaviorHints = behaviorHints; }

private:
    // In-class initialization ensures safer defaults and allows 'default' constructor
    QString m_id = {};
    QString m_version = {};
    QString m_name = {};
    QString m_description = {};
    QJsonArray m_resources = {};
    QStringList m_types = {};
    QList<CatalogDefinition> m_catalogs = {};
    QStringList m_idPrefixes = {};
    QString m_background = {};
    QString m_logo = {};
    QString m_contactEmail = {};
    QJsonObject m_behaviorHints = {};
};

#endif // ADDON_MANIFEST_H