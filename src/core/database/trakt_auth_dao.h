#ifndef TRAKT_AUTH_DAO_H
#define TRAKT_AUTH_DAO_H

#include <QSqlDatabase>
#include <QString>
#include <QDateTime>
#include <memory>

struct TraktAuthRecord
{
    int id;
    QString accessToken;
    QString refreshToken;
    int expiresIn;
    QDateTime createdAt;
    QDateTime expiresAt;
    QString username;  // nullable
    QString slug;      // nullable
};

class TraktAuthDao
{
public:
    explicit TraktAuthDao(QSqlDatabase database);
    
    std::unique_ptr<TraktAuthRecord> getTraktAuth();
    bool upsertTraktAuth(const TraktAuthRecord& auth);
    bool deleteTraktAuth();
    
private:
    QSqlDatabase m_database;
    
    TraktAuthRecord recordFromQuery(const QSqlQuery& query);
};

#endif // TRAKT_AUTH_DAO_H




