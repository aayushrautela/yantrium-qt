#include "trakt_auth_dao.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

TraktAuthDao::TraktAuthDao(QSqlDatabase database)
    : m_database(database)
{
}

std::unique_ptr<TraktAuthRecord> TraktAuthDao::getTraktAuth()
{
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM trakt_auth ORDER BY id DESC LIMIT 1");
    
    if (!query.exec()) {
        qWarning() << "Failed to get trakt auth:" << query.lastError().text();
        return nullptr;
    }
    
    if (!query.next()) {
        return nullptr;
    }
    
    return std::make_unique<TraktAuthRecord>(recordFromQuery(query));
}

bool TraktAuthDao::upsertTraktAuth(const TraktAuthRecord& auth)
{
    // Delete existing records first (only one auth record at a time)
    if (!deleteTraktAuth()) {
        qWarning() << "Failed to delete existing trakt auth";
        return false;
    }
    
    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT INTO trakt_auth (
            accessToken, refreshToken, expiresIn, createdAt, expiresAt, username, slug
        ) VALUES (?, ?, ?, ?, ?, ?, ?)
    )");
    
    query.addBindValue(auth.accessToken);
    query.addBindValue(auth.refreshToken);
    query.addBindValue(auth.expiresIn);
    query.addBindValue(auth.createdAt.toString(Qt::ISODate));
    query.addBindValue(auth.expiresAt.toString(Qt::ISODate));
    query.addBindValue(auth.username.isEmpty() ? QVariant() : QVariant(auth.username));
    query.addBindValue(auth.slug.isEmpty() ? QVariant() : QVariant(auth.slug));
    
    if (!query.exec()) {
        qWarning() << "Failed to insert trakt auth:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool TraktAuthDao::deleteTraktAuth()
{
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM trakt_auth");
    
    if (!query.exec()) {
        qWarning() << "Failed to delete trakt auth:" << query.lastError().text();
        return false;
    }
    
    return true;
}

TraktAuthRecord TraktAuthDao::recordFromQuery(const QSqlQuery& query)
{
    TraktAuthRecord record;
    record.id = query.value("id").toInt();
    record.accessToken = query.value("accessToken").toString();
    record.refreshToken = query.value("refreshToken").toString();
    record.expiresIn = query.value("expiresIn").toInt();
    record.createdAt = QDateTime::fromString(query.value("createdAt").toString(), Qt::ISODate);
    record.expiresAt = QDateTime::fromString(query.value("expiresAt").toString(), Qt::ISODate);
    record.username = query.value("username").toString();
    record.slug = query.value("slug").toString();
    return record;
}





