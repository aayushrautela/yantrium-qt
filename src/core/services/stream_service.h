#ifndef STREAM_SERVICE_H
#define STREAM_SERVICE_H

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QString>
#include "../models/stream_info.h"

class AddonRepository;
class TmdbDataService;
class LibraryService;

class StreamService : public QObject
{
    Q_OBJECT

public:
    explicit StreamService(QObject* parent = nullptr);
    ~StreamService();
    
    // Initialize with dependencies
    void setAddonRepository(AddonRepository* addonRepository);
    void setTmdbDataService(TmdbDataService* tmdbDataService);
    void setLibraryService(LibraryService* libraryService);
    
    // Get streams for a catalog item
    // itemData: QVariantMap with id, type, name, etc.
    // episodeId: Optional episode ID for TV shows (format: "S01E01")
    Q_INVOKABLE void getStreamsForItem(const QVariantMap& itemData, const QString& episodeId = QString());
    
signals:
    void streamsLoaded(const QVariantList& streams);
    void error(const QString& errorMessage);

private slots:
    void onStreamsFetched(const QString& type, const QString& id, const QJsonArray& streams);
    void onAddonError(const QString& errorMessage);
    void onTmdbMovieMetadataFetched(int tmdbId, const QJsonObject& data);
    void onTmdbTvMetadataFetched(int tmdbId, const QJsonObject& data);
    
private:
    struct PendingRequest {
        QString addonId;
        QString addonName;
        QString type;
        QString streamId;
    };
    
    QString extractImdbId(const QVariantMap& itemData);
    QString formatEpisodeId(int season, int episode);
    void fetchStreamsFromAddons();
    void processStreamsFromAddon(const QString& addonId, const QString& addonName, const QJsonArray& streams);
    void checkAllRequestsComplete();
    
    AddonRepository* m_addonRepository;
    TmdbDataService* m_tmdbDataService;
    LibraryService* m_libraryService;
    
    QVariantList m_allStreams;
    QList<PendingRequest> m_pendingRequests;
    int m_completedRequests;
    int m_totalRequests;
    
    // Current request context
    QVariantMap m_currentItemData;
    QString m_currentEpisodeId;
    QString m_currentImdbId;
    bool m_waitingForImdbId;
};

#endif // STREAM_SERVICE_H

