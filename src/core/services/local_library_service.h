#ifndef LOCAL_LIBRARY_SERVICE_H
#define LOCAL_LIBRARY_SERVICE_H

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <memory>
#include "../database/local_library_dao.h"
#include "../database/watch_history_dao.h"

class DatabaseManager;

class LocalLibraryService : public QObject
{
    Q_OBJECT

public:
    explicit LocalLibraryService(QObject* parent = nullptr);
    
    // Library methods
    Q_INVOKABLE void addToLibrary(const QVariantMap& item);
    Q_INVOKABLE void removeFromLibrary(const QString& contentId);
    Q_INVOKABLE void getLibraryItems();
    Q_INVOKABLE void isInLibrary(const QString& contentId);
    
    // Watch history methods
    Q_INVOKABLE void addToWatchHistory(const QVariantMap& item);
    Q_INVOKABLE void getWatchHistory(int limit = 100);
    Q_INVOKABLE void getWatchProgress(const QString& contentId, const QString& type, int season = -1, int episode = -1);
    Q_INVOKABLE void getWatchProgressByTmdbId(const QString& tmdbId, const QString& type, int season = -1, int episode = -1);

signals:
    void libraryItemsLoaded(const QVariantList& items);
    void libraryItemAdded(bool success);
    void libraryItemRemoved(bool success);
    void isInLibraryResult(bool inLibrary);
    void watchHistoryLoaded(const QVariantList& items);
    void watchProgressLoaded(const QVariantMap& progress);
    void error(const QString& message);

private:
    DatabaseManager* m_dbManager;
    std::unique_ptr<LocalLibraryDao> m_libraryDao;
    std::unique_ptr<WatchHistoryDao> m_historyDao;
    
    QVariantMap recordToVariantMap(const LocalLibraryRecord& record);
    LocalLibraryRecord variantMapToRecord(const QVariantMap& map);
    QVariantMap historyRecordToVariantMap(const WatchHistoryRecord& record);
    WatchHistoryRecord variantMapToHistoryRecord(const QVariantMap& map);
};

#endif // LOCAL_LIBRARY_SERVICE_H

