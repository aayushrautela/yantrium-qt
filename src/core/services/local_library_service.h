#ifndef LOCAL_LIBRARY_SERVICE_H
#define LOCAL_LIBRARY_SERVICE_H

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include "../database/database_manager.h"
#include "../database/local_library_dao.h"
#include "../database/watch_history_dao.h"

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

signals:
    void libraryItemsLoaded(const QVariantList& items);
    void libraryItemAdded(bool success);
    void libraryItemRemoved(bool success);
    void isInLibraryResult(bool inLibrary);
    void watchHistoryLoaded(const QVariantList& items);
    void error(const QString& message);

private:
    DatabaseManager& m_dbManager;
    LocalLibraryDao* m_libraryDao;
    WatchHistoryDao* m_historyDao;
    
    QVariantMap recordToVariantMap(const LocalLibraryRecord& record);
    LocalLibraryRecord variantMapToRecord(const QVariantMap& map);
    QVariantMap historyRecordToVariantMap(const WatchHistoryRecord& record);
    WatchHistoryRecord variantMapToHistoryRecord(const QVariantMap& map);
};

#endif // LOCAL_LIBRARY_SERVICE_H

