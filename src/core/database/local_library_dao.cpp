#include "local_library_dao.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

LocalLibraryDao::LocalLibraryDao(QSqlDatabase database)
    : m_database(database)
{
}

bool LocalLibraryDao::insertLibraryItem(const LocalLibraryRecord& item)
{
    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT OR REPLACE INTO local_library (
            contentId, type, title, year, posterUrl, backdropUrl, logoUrl,
            description, rating, addedAt, tmdbId, imdbId
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    
    query.addBindValue(item.contentId);
    query.addBindValue(item.type);
    query.addBindValue(item.title);
    query.addBindValue(item.year);
    query.addBindValue(item.posterUrl.isEmpty() ? QVariant() : QVariant(item.posterUrl));
    query.addBindValue(item.backdropUrl.isEmpty() ? QVariant() : QVariant(item.backdropUrl));
    query.addBindValue(item.logoUrl.isEmpty() ? QVariant() : QVariant(item.logoUrl));
    query.addBindValue(item.description.isEmpty() ? QVariant() : QVariant(item.description));
    query.addBindValue(item.rating.isEmpty() ? QVariant() : QVariant(item.rating));
    query.addBindValue(item.addedAt.toString(Qt::ISODate));
    query.addBindValue(item.tmdbId.isEmpty() ? QVariant() : QVariant(item.tmdbId));
    query.addBindValue(item.imdbId.isEmpty() ? QVariant() : QVariant(item.imdbId));
    
    if (!query.exec()) {
        qWarning() << "Failed to insert library item:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool LocalLibraryDao::removeLibraryItem(const QString& contentId)
{
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM local_library WHERE contentId = ?");
    query.addBindValue(contentId);
    
    if (!query.exec()) {
        qWarning() << "Failed to remove library item:" << query.lastError().text();
        return false;
    }
    
    return true;
}

QList<LocalLibraryRecord> LocalLibraryDao::getAllLibraryItems()
{
    QList<LocalLibraryRecord> items;
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM local_library ORDER BY addedAt DESC");
    
    if (!query.exec()) {
        qWarning() << "Failed to get library items:" << query.lastError().text();
        return items;
    }
    
    while (query.next()) {
        items.append(recordFromQuery(query));
    }
    
    return items;
}

std::unique_ptr<LocalLibraryRecord> LocalLibraryDao::getLibraryItem(const QString& contentId)
{
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM local_library WHERE contentId = ?");
    query.addBindValue(contentId);
    
    if (!query.exec()) {
        qWarning() << "Failed to get library item:" << query.lastError().text();
        return nullptr;
    }
    
    if (!query.next()) {
        return nullptr;
    }
    
    return std::make_unique<LocalLibraryRecord>(recordFromQuery(query));
}

bool LocalLibraryDao::isInLibrary(const QString& contentId)
{
    QSqlQuery query(m_database);
    query.prepare("SELECT COUNT(*) FROM local_library WHERE contentId = ?");
    query.addBindValue(contentId);
    
    if (!query.exec()) {
        qWarning() << "Failed to check library item:" << query.lastError().text();
        return false;
    }
    
    if (!query.next()) {
        return false;
    }
    
    return query.value(0).toInt() > 0;
}

LocalLibraryRecord LocalLibraryDao::recordFromQuery(const QSqlQuery& query)
{
    LocalLibraryRecord record;
    record.id = query.value("id").toInt();
    record.contentId = query.value("contentId").toString();
    record.type = query.value("type").toString();
    record.title = query.value("title").toString();
    record.year = query.value("year").toInt();
    record.posterUrl = query.value("posterUrl").toString();
    record.backdropUrl = query.value("backdropUrl").toString();
    record.logoUrl = query.value("logoUrl").toString();
    record.description = query.value("description").toString();
    record.rating = query.value("rating").toString();
    record.addedAt = QDateTime::fromString(query.value("addedAt").toString(), Qt::ISODate);
    record.tmdbId = query.value("tmdbId").toString();
    record.imdbId = query.value("imdbId").toString();
    return record;
}


