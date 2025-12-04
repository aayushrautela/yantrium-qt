#ifndef CATALOG_DEFINITION_H
#define CATALOG_DEFINITION_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>

// Removed <optional> as it was unused in your code.
// If you intend to use std::optional later, add it back.

class CatalogDefinition
{
public:
    // Modern: Use 'default' to use the in-class initializers below
    CatalogDefinition() = default;

    CatalogDefinition(const QString& type,
                      const QString& id = {},
                      const QString& name = {},
                      int pageSize = 0,
                      const QList<QJsonObject>& extra = {});
    
    [[nodiscard]] static CatalogDefinition fromJson(const QJsonObject& json);
    [[nodiscard]] QJsonObject toJson() const;
    
    [[nodiscard]] QString type() const { return m_type; }
    void setType(const QString& type) { m_type = type; }
    
    [[nodiscard]] QString id() const { return m_id; }
    void setId(const QString& id) { m_id = id; }
    
    [[nodiscard]] QString name() const { return m_name; }
    void setName(const QString& name) { m_name = name; }
    
    [[nodiscard]] int pageSize() const { return m_pageSize; }
    void setPageSize(int pageSize) { m_pageSize = pageSize; }
    
    [[nodiscard]] QList<QJsonObject> extra() const { return m_extra; }
    void setExtra(const QList<QJsonObject>& extra) { m_extra = extra; }
    
private:
    // In-class initialization (C++11/14+)
    // Ensures the object is always in a valid state without explicit constructor code
    QString m_type = {};
    QString m_id = {};
    QString m_name = {};
    int m_pageSize = 0;
    QList<QJsonObject> m_extra = {};
};

#endif // CATALOG_DEFINITION_H