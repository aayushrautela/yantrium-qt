#ifndef TORRENT_SERVICE_H
#define TORRENT_SERVICE_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QtQmlIntegration/qqmlintegration.h>

class TorrentStreamServer;

/**
 * @brief Service for managing torrent streams
 * 
 * Provides a high-level interface for converting magnet links to streamable URLs
 * that can be played by the MDK player.
 */
class TorrentService : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit TorrentService(QObject* parent = nullptr);
    ~TorrentService();

    /**
     * @brief Check if torrent support is available
     */
    Q_INVOKABLE bool isAvailable() const;

    /**
     * @brief Convert a magnet link or infoHash to a streamable HTTP URL
     * @param magnetLinkOrHash Magnet link or infoHash
     * @param fileIndex File index for multi-file torrents (-1 for single file or auto-detect video)
     * @return Streamable HTTP URL, or empty if failed
     */
    Q_INVOKABLE QString getStreamUrl(const QString& magnetLinkOrHash, int fileIndex = -1);

    /**
     * @brief Check if a URL is a magnet link or infoHash
     */
    Q_INVOKABLE bool isMagnetLink(const QString& url) const;

    /**
     * @brief Get download progress for a stream URL (0.0 to 1.0)
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

    /**
     * @brief Stop and remove a torrent
     */
    Q_INVOKABLE void removeTorrent(const QString& streamUrl);

signals:
    void streamReady(const QString& streamUrl);
    void streamError(const QString& streamUrl, const QString& error);
    void progressChanged(const QString& streamUrl, double progress);

private:
    QString normalizeMagnetLink(const QString& input) const;
    bool m_available;
    TorrentStreamServer* m_streamServer;
};

#endif // TORRENT_SERVICE_H


