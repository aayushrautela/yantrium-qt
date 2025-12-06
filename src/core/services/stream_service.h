#ifndef STREAM_SERVICE_H
#define STREAM_SERVICE_H

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QString>
#include <memory>
#include <QtQmlIntegration/qqmlintegration.h>
#include "../models/stream_info.h"
#include "interfaces/istream_service.h"

class AddonRepository;
class TmdbDataService;
class LibraryService;

class StreamService : public QObject, public IStreamService
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit StreamService(
        std::shared_ptr<AddonRepository> addonRepository,
        std::shared_ptr<TmdbDataService> tmdbDataService,
        LibraryService* libraryService = nullptr,
        QObject* parent = nullptr);
    ~StreamService() override = default;
    
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
    
    std::shared_ptr<AddonRepository> m_addonRepository;
    std::shared_ptr<TmdbDataService> m_tmdbDataService;
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

