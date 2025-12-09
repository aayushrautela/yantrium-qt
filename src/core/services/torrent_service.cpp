#include "torrent_service.h"
#include "torrent_stream_server.h"
#include "logging_service.h"
#include <QUrl>
#include <QRegularExpression>

TorrentService::TorrentService(QObject* parent)
    : QObject(parent)
    , m_available(false)
    , m_streamServer(nullptr)
{
#ifdef TORRENT_SUPPORT_ENABLED
    m_available = true;
    m_streamServer = new TorrentStreamServer(this);
    
    // Start the streaming server
    if (m_streamServer->startServer(0)) {
        LoggingService::logInfo("TorrentService", 
            QString("Torrent streaming server started at %1").arg(m_streamServer->getBaseUrl()));
        
        connect(m_streamServer, &TorrentStreamServer::torrentReady, 
                this, &TorrentService::streamReady);
        connect(m_streamServer, &TorrentStreamServer::torrentError, 
                this, [this](const QString& url, const QString& error) {
                    emit streamError(url, error);
                });
        connect(m_streamServer, &TorrentStreamServer::progressChanged, 
                this, &TorrentService::progressChanged);
    } else {
        LoggingService::logError("TorrentService", "Failed to start torrent streaming server");
        m_available = false;
    }
#else
    LoggingService::logWarning("TorrentService", "Torrent support not compiled in (libtorrent not found)");
    m_available = false;
#endif
}

TorrentService::~TorrentService() = default;

bool TorrentService::isAvailable() const
{
    return m_available;
}

QString TorrentService::getStreamUrl(const QString& magnetLinkOrHash, int fileIndex)
{
    if (!m_available || !m_streamServer) {
        LoggingService::logWarning("TorrentService", "Torrent service not available");
        return QString();
    }

    QString normalized = normalizeMagnetLink(magnetLinkOrHash);
    if (normalized.isEmpty()) {
        LoggingService::logError("TorrentService", "Invalid magnet link or hash");
        return QString();
    }

    QString streamUrl = m_streamServer->addMagnetLink(normalized, fileIndex);
    if (!streamUrl.isEmpty()) {
        LoggingService::logInfo("TorrentService", 
            QString("Added torrent, stream URL: %1").arg(streamUrl));
    } else {
        LoggingService::logError("TorrentService", "Failed to add magnet link");
    }

    return streamUrl;
}

bool TorrentService::isMagnetLink(const QString& url) const
{
    if (url.startsWith("magnet:", Qt::CaseInsensitive)) {
        return true;
    }
    
    // Check if it's just an infoHash (40 hex chars)
    QRegularExpression hashRegex("^[0-9a-fA-F]{40}$");
    if (hashRegex.match(url).hasMatch()) {
        return true;
    }
    
    return false;
}

double TorrentService::getProgress(const QString& streamUrl) const
{
    if (!m_available || !m_streamServer) {
        return 0.0;
    }
    return m_streamServer->getProgress(streamUrl);
}

qint64 TorrentService::getDownloadSpeed(const QString& streamUrl) const
{
    if (!m_available || !m_streamServer) {
        return 0;
    }
    return m_streamServer->getDownloadSpeed(streamUrl);
}

bool TorrentService::isReady(const QString& streamUrl) const
{
    if (!m_available || !m_streamServer) {
        return false;
    }
    return m_streamServer->isReady(streamUrl);
}

void TorrentService::removeTorrent(const QString& streamUrl)
{
    if (!m_available || !m_streamServer) {
        return;
    }
    m_streamServer->removeTorrent(streamUrl);
}

QString TorrentService::normalizeMagnetLink(const QString& input) const
{
    QString trimmed = input.trimmed();
    
    // If it's already a magnet link, return as-is
    if (trimmed.startsWith("magnet:", Qt::CaseInsensitive)) {
        return trimmed;
    }
    
    // If it's just an infoHash, convert to magnet link
    QRegularExpression hashRegex("^[0-9a-fA-F]{40}$");
    if (hashRegex.match(trimmed).hasMatch()) {
        // Convert to magnet link with common trackers
        static const QStringList trackers = {
            "udp://tracker.opentrackr.org:1337/announce",
            "udp://9.rarbg.com:2810/announce",
            "udp://tracker.openbittorrent.com:6969/announce",
            "udp://tracker.torrent.eu.org:451/announce",
            "udp://open.stealth.si:80/announce",
            "udp://tracker.leechers-paradise.org:6969/announce",
            "udp://tracker.coppersurfer.tk:6969/announce",
            "udp://tracker.internetwarriors.net:1337/announce"
        };
        
        QString trackersString;
        for (const QString& tracker : trackers) {
            trackersString += "&tr=" + QUrl::toPercentEncoding(tracker);
        }
        
        return QString("magnet:?xt=urn:btih:%1%2").arg(trimmed, trackersString);
    }
    
    return QString();
}


