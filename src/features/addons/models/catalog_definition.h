#ifndef CATALOG_DEFINITION_H
#define CATALOG_DEFINITION_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <optional>

class CatalogDefinition
{
public:
    CatalogDefinition();
    CatalogDefinition(const QString& type,
                     const QString& id = QString(),
                     const QString& name = QString(),
                     int pageSize = 0,
                     const QList<QJsonObject>& extra = QList<QJsonObject>());
    
    static CatalogDefinition fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    QString type() const { return m_type; }
    void setType(const QString& type) { m_type = type; }
    
    QString id() const { return m_id; }
    void setId(const QString& id) { m_id = id; }
    
    QString name() const { return m_name; }
    void setName(const QString& name) { m_name = name; }
    
    int pageSize() const { return m_pageSize; }
    void setPageSize(int pageSize) { m_pageSize = pageSize; }
    
    QList<QJsonObject> extra() const { return m_extra; }
    void setExtra(const QList<QJsonObject>& extra) { m_extra = extra; }
    
private:
    QString m_type;
    QString m_id;
    QString m_name;
    int m_pageSize;
    QList<QJsonObject> m_extra;
};

#endif // CATALOG_DEFINITION_H



