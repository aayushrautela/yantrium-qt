#ifndef LOCAL_LIBRARY_DAO_H
#define LOCAL_LIBRARY_DAO_H

#include <QSqlDatabase>
#include <QString>
#include <QDateTime>
#include <QList>
#include <memory>

struct LocalLibraryRecord
{
    int id;
    QString contentId;
    QString type;
    QString title;
    int year;
    QString posterUrl;
    QString backdropUrl;
    QString logoUrl;
    QString description;
    QString rating;
    QDateTime addedAt;
    QString tmdbId;
    QString imdbId;
};

class LocalLibraryDao
{
public:
    explicit LocalLibraryDao();
    
    bool insertLibraryItem(const LocalLibraryRecord& item);
    bool removeLibraryItem(const QString& contentId);
    QList<LocalLibraryRecord> getAllLibraryItems();
    std::unique_ptr<LocalLibraryRecord> getLibraryItem(const QString& contentId);
    bool isInLibrary(const QString& contentId);
    
private:
    QSqlDatabase m_database;
    
    LocalLibraryRecord recordFromQuery(const QSqlQuery& query);
};

#endif // LOCAL_LIBRARY_DAO_H


