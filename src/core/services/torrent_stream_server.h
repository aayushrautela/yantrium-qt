#ifndef TORRENT_STREAM_SERVER_H
#define TORRENT_STREAM_SERVER_H

#include <QObject>
#include <QString>
#include <memory>
#include <QtQmlIntegration/qqmlintegration.h>

#ifdef TORRENT_SUPPORT_ENABLED
#ifdef HAVE_QHTTPSERVER
#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#endif
#include <libtorrent/session.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/read_resume_data.hpp>
#include <libtorrent/write_resume_data.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/torrent_info.hpp>
#endif

class TorrentStreamServer : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit TorrentStreamServer(QObject* parent = nullptr);
    ~TorrentStreamServer();

    /**
     * @brief Start the HTTP server for streaming torrent content
     * @param port Port to listen on (0 = auto-assign)
     * @return True if started successfully
     */
    Q_INVOKABLE bool startServer(quint16 port = 0);

    /**
     * @brief Stop the HTTP server
     */
    Q_INVOKABLE void stopServer();

    /**
     * @brief Get the base URL for streaming
     */
    Q_INVOKABLE QString getBaseUrl() const;

    /**
     * @brief Add a magnet link and return a streamable URL
     * @param magnetLink Magnet link or infoHash
     * @param fileIndex File index in torrent (for multi-file torrents)
     * @return Streamable HTTP URL, or empty if failed
     */
    Q_INVOKABLE QString addMagnetLink(const QString& magnetLink, int fileIndex = -1);

    /**
     * @brief Remove a torrent from the session
     * @param streamUrl The stream URL returned by addMagnetLink
     */
    Q_INVOKABLE void removeTorrent(const QString& streamUrl);

    /**
     * @brief Get download progress (0.0 to 1.0)
     */
    Q_INVOKABLE double getProgress(const QString& streamUrl) const;

    /**
     * @brief Get download speed in bytes per second
     */
    Q_INVOKABLE qint64 getDownloadSpeed(const QString& streamUrl) const;

    /**
     * @brief Check if torrent is ready for streaming
     */
    Q_INVOKABLE bool isReady(const QString& streamUrl) const;

signals:
    void torrentAdded(const QString& streamUrl);
    void torrentReady(const QString& streamUrl);
    void torrentError(const QString& streamUrl, const QString& error);
    void progressChanged(const QString& streamUrl, double progress);

private slots:
    void onTorrentAlert();

private:
#ifdef TORRENT_SUPPORT_ENABLED
    struct TorrentInfo {
        libtorrent::torrent_handle handle;
        QString magnetLink;
        int fileIndex;
        bool isReady;
        double progress;
        qint64 downloadSpeed;
    };

    void processTorrentAlerts();
#ifdef HAVE_QHTTPSERVER
    QHttpServerResponse handleRequest(const QHttpServerRequest& request);
    std::unique_ptr<QHttpServer> m_server;
#endif
    QString generateStreamPath(const QString& torrentId, int fileIndex);
    libtorrent::torrent_handle findTorrentByStreamUrl(const QString& streamUrl) const;
    QString getTorrentId(const QString& streamUrl) const;
    libtorrent::session m_session;
    QMap<QString, TorrentInfo> m_torrents; // streamUrl -> TorrentInfo
    QMap<QString, QString> m_streamUrlToMagnet; // streamUrl -> magnetLink
    quint16 m_port;
    QString m_baseUrl;
    QTimer* m_alertTimer;
#else
    // Stub implementations when libtorrent is not available
    QString m_baseUrl;
    quint16 m_port;
#endif
};

#endif // TORRENT_STREAM_SERVER_H


