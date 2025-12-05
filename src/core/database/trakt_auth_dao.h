#ifndef TRAKT_AUTH_DAO_H
#define TRAKT_AUTH_DAO_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QDateTime>
#include <memory>
#include <string_view>

#include "database_manager.h"

struct TraktAuthRecord
{
    int id = 0;
    QString accessToken;
    QString refreshToken;
    int expiresIn = 0;
    QDateTime createdAt;
    QDateTime expiresAt;
    QString username;  // nullable
    QString slug;      // nullable

    // Default constructor with modern initialization
    TraktAuthRecord() = default;

    // Constructor for authenticated records
    TraktAuthRecord(QString accessToken, QString refreshToken, int expiresIn,
                   QDateTime createdAt, QDateTime expiresAt,
                   QString username = {}, QString slug = {})
        : accessToken(std::move(accessToken)), refreshToken(std::move(refreshToken)),
          expiresIn(expiresIn), createdAt(createdAt), expiresAt(expiresAt),
          username(std::move(username)), slug(std::move(slug)) {}
};

class TraktAuthDao
{
public:
    // Modern constructor - explicit and noexcept
    explicit TraktAuthDao() noexcept;

    // Critical authentication operations - mark as [[nodiscard]]
    [[nodiscard]] std::unique_ptr<TraktAuthRecord> getTraktAuth();
    [[nodiscard]] bool upsertTraktAuth(const TraktAuthRecord& auth);
    [[nodiscard]] bool deleteTraktAuth();

private:
    // Helper method - const and noexcept where safe
    [[nodiscard]] TraktAuthRecord recordFromQuery(const QSqlQuery& query) const noexcept;

    // Database connection getter - modern approach (replaces member variable)
    [[nodiscard]] static QSqlDatabase getDatabase() noexcept {
        return QSqlDatabase::database(DatabaseManager::CONNECTION_NAME);
    }
};

#endif // TRAKT_AUTH_DAO_H






