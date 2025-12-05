#ifndef LOCAL_LIBRARY_DAO_H
#define LOCAL_LIBRARY_DAO_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QDateTime>
#include <QList>
#include <memory>
#include <string_view>

#include "database_manager.h"

struct LocalLibraryRecord
{
    int id = 0;
    QString contentId;
    QString type;
    QString title;
    int year = 0;
    QString posterUrl;
    QString backdropUrl;
    QString logoUrl;
    QString description;
    QString rating;
    QDateTime addedAt;
    QString tmdbId;
    QString imdbId;

    // Default constructor with modern initialization
    LocalLibraryRecord() = default;

    // Constructor for adding new library items
    LocalLibraryRecord(QString contentId, QString type, QString title,
                      int year = 0, QDateTime addedAt = QDateTime::currentDateTime())
        : contentId(std::move(contentId)), type(std::move(type)),
          title(std::move(title)), year(year), addedAt(addedAt) {}

    // Check if record is valid (has been loaded from database)
    [[nodiscard]] bool isValid() const noexcept { return id > 0; }

    // Check if item has basic required fields
    [[nodiscard]] bool hasRequiredFields() const noexcept {
        return !contentId.isEmpty() && !type.isEmpty() && !title.isEmpty();
    }
};

class LocalLibraryDao
{
public:
    // Modern constructor - explicit and noexcept
    explicit LocalLibraryDao() noexcept;

    // Critical library operations - mark as [[nodiscard]]
    [[nodiscard]] bool insertLibraryItem(const LocalLibraryRecord& item);
    [[nodiscard]] bool removeLibraryItem(std::string_view contentId);
    [[nodiscard]] QList<LocalLibraryRecord> getAllLibraryItems();
    [[nodiscard]] std::unique_ptr<LocalLibraryRecord> getLibraryItem(std::string_view contentId);
    [[nodiscard]] bool isInLibrary(std::string_view contentId) const;

private:
    // Helper method - const and noexcept where safe
    [[nodiscard]] LocalLibraryRecord recordFromQuery(const QSqlQuery& query) const noexcept;

    // Database connection getter - modern approach (replaces member variable)
    [[nodiscard]] static QSqlDatabase getDatabase() noexcept {
        return QSqlDatabase::database(DatabaseManager::CONNECTION_NAME);
    }
};

#endif // LOCAL_LIBRARY_DAO_H


