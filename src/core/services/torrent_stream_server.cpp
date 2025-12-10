#include "torrent_stream_server.h"
#include "logging_service.h"
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QRegularExpression>
#include <QByteArray>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>
#include <QDir>
#include <QHostAddress>
#include <QTcpServer>
#include <QThread>

#ifdef TORRENT_SUPPORT_ENABLED
#ifdef HAVE_QHTTPSERVER
#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QHttpHeaders>
#endif
#include <libtorrent/session.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/read_resume_data.hpp>
#include <libtorrent/write_resume_data.hpp>
#include <libtorrent/error_code.hpp>
// #include <libtorrent/hex.hpp> // Removed to prevent usage of deprecated/unlinked header
#include <libtorrent/string_view.hpp>
#include <algorithm>
#include <fstream>
#endif

TorrentStreamServer::TorrentStreamServer(QObject* parent)
    : QObject(parent)
    , m_port(0)
#ifdef TORRENT_SUPPORT_ENABLED
    , m_alertTimer(nullptr)
#endif
{
#ifdef TORRENT_SUPPORT_ENABLED
    // Configure libtorrent session
    libtorrent::settings_pack settings;
    settings.set_int(libtorrent::settings_pack::alert_mask, 
        libtorrent::alert::error_notification | 
        libtorrent::alert::status_notification |
        libtorrent::alert::torrent_log_notification);
    settings.set_bool(libtorrent::settings_pack::enable_dht, true);
    settings.set_bool(libtorrent::settings_pack::enable_lsd, true);
    settings.set_bool(libtorrent::settings_pack::enable_natpmp, true);
    settings.set_bool(libtorrent::settings_pack::enable_upnp, true);
    settings.set_int(libtorrent::settings_pack::download_rate_limit, 0); // Unlimited
    settings.set_int(libtorrent::settings_pack::upload_rate_limit, 0); // Unlimited
    settings.set_int(libtorrent::settings_pack::active_downloads, 10);
    settings.set_int(libtorrent::settings_pack::active_seeds, 10);
    
    m_session.apply_settings(settings);
    
    // Set up alert processing timer
    m_alertTimer = new QTimer(this);
    connect(m_alertTimer, &QTimer::timeout, this, &TorrentStreamServer::onTorrentAlert);
    m_alertTimer->start(500); // Check alerts every 500ms
#endif
}

TorrentStreamServer::~TorrentStreamServer()
{
    stopServer();
}

bool TorrentStreamServer::startServer(quint16 port)
{
#ifdef TORRENT_SUPPORT_ENABLED
#ifdef HAVE_QHTTPSERVER
    if (m_server) {
        LoggingService::logWarning("TorrentStreamServer", "Server already running");
        return true;
    }
    
    m_server = std::make_unique<QHttpServer>();
    
    // Handle stream requests
    m_server->route("/stream/<arg>", [this](const QString& /*streamId*/, const QHttpServerRequest& request) {
        return handleRequest(request);
    });
    
    // Handle root requests (redirect to stream)
    m_server->route("/", [this](const QHttpServerRequest& /*request*/) {
        return QHttpServerResponse("text/plain", "Torrent Stream Server");
    });
    
    // Use bind() for newer Qt6HttpServer API, fallback to listen() for older versions
    #if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    // Newer API: bind() requires a QTcpServer (bind() takes ownership)
    auto tcpServer = new QTcpServer();
    if (!tcpServer->listen(QHostAddress::LocalHost, port)) {
        LoggingService::logError("TorrentStreamServer", "Failed to start TCP server");
        delete tcpServer;
        return false;
    }
    if (!m_server->bind(tcpServer)) {
        LoggingService::logError("TorrentStreamServer", "Failed to bind HTTP server");
        delete tcpServer;
        return false;
    }
    // bind() takes ownership, so don't delete tcpServer
    auto ports = m_server->serverPorts();
    m_port = ports.isEmpty() ? tcpServer->serverPort() : ports.first();
    #else
    // Older API: listen() directly on QHttpServer
    if (!m_server->listen(QHostAddress::LocalHost, port)) {
        LoggingService::logError("TorrentStreamServer", "Failed to start HTTP server");
        return false;
    }
    m_port = m_server->serverPort();
    #endif
    m_baseUrl = QString("http://localhost:%1").arg(m_port);
    
    LoggingService::logInfo("TorrentStreamServer", 
        QString("HTTP server started on port %1").arg(m_port));
    return true;
#else
    Q_UNUSED(port);
    LoggingService::logWarning("TorrentStreamServer", "Qt6HttpServer not available - torrent streaming disabled");
    return false;
#endif
#else
    Q_UNUSED(port);
    LoggingService::logWarning("TorrentStreamServer", "Torrent support not available");
    return false;
#endif
}

void TorrentStreamServer::stopServer()
{
#ifdef TORRENT_SUPPORT_ENABLED
    if (m_alertTimer) {
        m_alertTimer->stop();
    }
    
    // Remove all torrents
    for (auto torrentIt = m_torrents.cbegin(); torrentIt != m_torrents.cend(); ++torrentIt) {
        m_session.remove_torrent(torrentIt.value().handle);
    }
    m_torrents.clear();
    m_streamUrlToMagnet.clear();
    
#ifdef HAVE_QHTTPSERVER
    if (m_server) {
        m_server.reset();
    }
#endif
    
    m_port = 0;
    m_baseUrl.clear();
#endif
}

QString TorrentStreamServer::getBaseUrl() const
{
    return m_baseUrl;
}

QString TorrentStreamServer::addMagnetLink(const QString& magnetLink, int fileIndex)
{
#ifdef TORRENT_SUPPORT_ENABLED
#ifdef HAVE_QHTTPSERVER
    if (!m_server) {
        LoggingService::logError("TorrentStreamServer", "Server not started");
        return QString();
    }
    
    libtorrent::error_code ec;
    libtorrent::add_torrent_params params;
    
    // Parse magnet link
    params = libtorrent::parse_magnet_uri(magnetLink.toStdString(), ec);
    if (ec) {
        LoggingService::logError("TorrentStreamServer", 
            QString("Failed to parse magnet link: %1").arg(ec.message().c_str()));
        return QString();
    }
    
    // Set save path (we'll stream from memory, but need a temp path)
    QString tempPath = QDir::temp().absoluteFilePath("yantrium-torrents");
    QDir().mkpath(tempPath);
    params.save_path = tempPath.toStdString();
    
    // Add torrent to session
    libtorrent::torrent_handle handle = m_session.add_torrent(params, ec);
    if (ec) {
        LoggingService::logError("TorrentStreamServer", 
            QString("Failed to add torrent: %1").arg(ec.message().c_str()));
        return QString();
    }
    
    // Generate stream URL
    libtorrent::sha1_hash hash = handle.info_hash();
    // FIX: Replaced libtorrent::to_hex with Qt's native hex conversion to avoid LNK2019
    QString torrentId = QByteArray::fromStdString(hash.to_string()).toHex();
    QString streamPath = generateStreamPath(torrentId, fileIndex);
    QString streamUrl = m_baseUrl + streamPath;
    
    // Store torrent info
    TorrentInfo info;
    info.handle = handle;
    info.magnetLink = magnetLink;
    info.fileIndex = fileIndex;
    info.isReady = false;
    info.progress = 0.0;
    info.downloadSpeed = 0;
    
    m_torrents[streamUrl] = info;
    m_streamUrlToMagnet[streamUrl] = magnetLink;
    
    LoggingService::logInfo("TorrentStreamServer", 
        QString("Added torrent, stream URL: %1").arg(streamUrl));
    
    emit torrentAdded(streamUrl);
    return streamUrl;
#else
    Q_UNUSED(magnetLink);
    Q_UNUSED(fileIndex);
    LoggingService::logError("TorrentStreamServer", "Qt6HttpServer not available - torrent streaming disabled");
    return QString();
#endif
#else
    Q_UNUSED(magnetLink);
    Q_UNUSED(fileIndex);
    return QString();
#endif
}

void TorrentStreamServer::removeTorrent(const QString& streamUrl)
{
#ifdef TORRENT_SUPPORT_ENABLED
    if (m_torrents.contains(streamUrl)) {
        m_session.remove_torrent(m_torrents.value(streamUrl).handle);
        m_torrents.remove(streamUrl);
        m_streamUrlToMagnet.remove(streamUrl);
        LoggingService::logInfo("TorrentStreamServer", 
            QString("Removed torrent: %1").arg(streamUrl));
    }
#else
    Q_UNUSED(streamUrl);
#endif
}

double TorrentStreamServer::getProgress(const QString& streamUrl) const
{
#ifdef TORRENT_SUPPORT_ENABLED
    if (m_torrents.contains(streamUrl)) {
        return m_torrents.value(streamUrl).progress;
    }
#endif
    Q_UNUSED(streamUrl);
    return 0.0;
}

qint64 TorrentStreamServer::getDownloadSpeed(const QString& streamUrl) const
{
#ifdef TORRENT_SUPPORT_ENABLED
    if (m_torrents.contains(streamUrl)) {
        return m_torrents.value(streamUrl).downloadSpeed;
    }
#endif
    Q_UNUSED(streamUrl);
    return 0;
}

bool TorrentStreamServer::isReady(const QString& streamUrl) const
{
#ifdef TORRENT_SUPPORT_ENABLED
    if (m_torrents.contains(streamUrl)) {
        return m_torrents.value(streamUrl).isReady;
    }
#endif
    Q_UNUSED(streamUrl);
    return false;
}

void TorrentStreamServer::onTorrentAlert()
{
#ifdef TORRENT_SUPPORT_ENABLED
    processTorrentAlerts();
#endif
}

#ifdef TORRENT_SUPPORT_ENABLED
void TorrentStreamServer::processTorrentAlerts()
{
    std::vector<libtorrent::alert*> alerts;
    m_session.pop_alerts(&alerts);
    
    for (libtorrent::alert* alert : alerts) {
        if (auto* ta = libtorrent::alert_cast<libtorrent::add_torrent_alert>(alert)) {
            // Torrent metadata downloaded
            libtorrent::sha1_hash hash = ta->handle.info_hash();
            // FIX: Replaced libtorrent::to_hex
            QString torrentId = QByteArray::fromStdString(hash.to_string()).toHex();

            // Find stream URL for this torrent
            QString streamUrl;
            for (auto torrentIt = m_torrents.cbegin(); torrentIt != m_torrents.cend(); ++torrentIt) {
                if (torrentIt.value().handle.info_hash() == ta->handle.info_hash()) {
                    streamUrl = torrentIt.key();
                    break;
                }
            }

            if (!streamUrl.isEmpty()) {
                LoggingService::logInfo("TorrentStreamServer",
                    QString("Torrent metadata downloaded: %1").arg(streamUrl));
            }
        }
        else if (auto* sa = libtorrent::alert_cast<libtorrent::state_update_alert>(alert)) {
            // Update torrent states
            for (const auto& status : sa->status) {
                libtorrent::sha1_hash hash = status.handle.info_hash();
                // FIX: Replaced libtorrent::to_hex
                QString torrentId = QByteArray::fromStdString(hash.to_string()).toHex();

                // Find stream URL
                QString streamUrl;
                for (auto it = m_torrents.begin(); it != m_torrents.end(); ++it) {
                    if (it.value().handle.info_hash() == status.handle.info_hash()) {
                        streamUrl = it.key();
                        TorrentInfo& info = it.value();

                        // Update progress
                        double progress = status.progress;
                        if (progress != info.progress) {
                            info.progress = progress;
                            emit progressChanged(streamUrl, progress);
                        }

                        // Update download speed
                        info.downloadSpeed = status.download_payload_rate;

                        // Check if ready (have enough pieces to start streaming)
                        bool wasReady = info.isReady;
                        info.isReady = (progress > 0.01); // 1% downloaded

                        if (!wasReady && info.isReady) {
                            emit torrentReady(streamUrl);
                        }

                        break;
                    }
                }
            }
        }
        else if (auto* ea = libtorrent::alert_cast<libtorrent::torrent_error_alert>(alert)) {
            libtorrent::sha1_hash hash = ea->handle.info_hash();
            // FIX: Replaced libtorrent::to_hex
            QString torrentId = QByteArray::fromStdString(hash.to_string()).toHex();
            
            QString streamUrl;
            for (auto torrentIt = m_torrents.cbegin(); torrentIt != m_torrents.cend(); ++torrentIt) {
                if (torrentIt.value().handle.info_hash() == ea->handle.info_hash()) {
                    streamUrl = torrentIt.key();
                    break;
                }
            }
            
            if (!streamUrl.isEmpty()) {
                QString errorMsg = QString::fromStdString(ea->error.message());
                LoggingService::logError("TorrentStreamServer", 
                    QString("Torrent error: %1 - %2").arg(streamUrl, errorMsg));
                emit torrentError(streamUrl, errorMsg);
            }
        }
    }
}

#ifdef HAVE_QHTTPSERVER
QHttpServerResponse TorrentStreamServer::handleRequest(const QHttpServerRequest& request)
{
    QString path = request.url().path();
    
    // Extract torrent ID and file index from path
    // Format: /stream/<torrentId>[/<fileIndex>]
    QRegularExpression pathRegex("/stream/([^/]+)(?:/(\\d+))?");
    QRegularExpressionMatch match = pathRegex.match(path);
    
    if (!match.hasMatch()) {
        return QHttpServerResponse(QHttpServerResponse::StatusCode::NotFound);
    }
    
    QString torrentId = match.captured(1);
    int fileIndex = match.captured(2).toInt();
    
    // Find torrent by ID
    libtorrent::torrent_handle handle;
    TorrentInfo* info = nullptr;
    
    for (auto it = m_torrents.begin(); it != m_torrents.end(); ++it) {
        libtorrent::sha1_hash hash = it.value().handle.info_hash();
        // FIX: Replaced libtorrent::to_hex
        QString id = QByteArray::fromStdString(hash.to_string()).toHex();
        if (id == torrentId) {
            handle = it.value().handle;
            info = &it.value();
            break;
        }
    }
    
    if (!handle.is_valid() || !info) {
        return QHttpServerResponse(QHttpServerResponse::StatusCode::NotFound);
    }
    
    // Get torrent status
    libtorrent::torrent_status status = handle.status();
    
    if (!status.has_metadata) {
        return QHttpServerResponse("text/plain", "Torrent metadata not available yet", 
            QHttpServerResponse::StatusCode::ServiceUnavailable);
    }
    
    // Get file index (use provided or auto-detect video file)
    int actualFileIndex = fileIndex;
    auto ti_ptr = handle.torrent_file();
    if (!ti_ptr) {
        return QHttpServerResponse(QHttpServerResponse::StatusCode::NotFound);
    }
    const libtorrent::torrent_info& ti = *ti_ptr;
    
    if (actualFileIndex < 0) {
        // Auto-detect largest video file
        qint64 maxSize = 0;
        for (int i = 0; i < ti.num_files(); ++i) {
            const libtorrent::file_storage& files = ti.files();
            libtorrent::file_index_t fileIdx(i);
            QString path = QString::fromStdString(files.file_path(fileIdx));
            qint64 fileSize = files.file_size(fileIdx);
            if (path.endsWith(".mp4") || path.endsWith(".mkv") || path.endsWith(".avi") ||
                path.endsWith(".mov") || path.endsWith(".webm")) {
                if (fileSize > maxSize) {
                    maxSize = fileSize;
                    actualFileIndex = i;
                }
            }
        }
        
        if (actualFileIndex < 0) {
            actualFileIndex = 0; // Fallback to first file
        }
    }
    
    // Check if file exists in torrent
    if (actualFileIndex >= ti.num_files()) {
        return QHttpServerResponse(QHttpServerResponse::StatusCode::NotFound);
    }
    
    // Convert to libtorrent::file_index_t for libtorrent API calls
    libtorrent::file_index_t ltFileIndex(actualFileIndex);
    
    // Handle range requests for video streaming
    QString rangeHeader = request.value("Range");
    qint64 start = 0;
    qint64 end = -1;
    
    if (!rangeHeader.isEmpty()) {
        QRegularExpression rangeRegex("bytes=(\\d+)-(\\d*)");
        QRegularExpressionMatch rangeMatch = rangeRegex.match(rangeHeader);
        if (rangeMatch.hasMatch()) {
            start = rangeMatch.captured(1).toLongLong();
            QString endStr = rangeMatch.captured(2);
            if (!endStr.isEmpty()) {
                end = endStr.toLongLong();
            }
        }
    }
    
    // Get file size
    const libtorrent::file_storage& files = ti.files();
    qint64 fileSize = files.file_size(ltFileIndex);
    
    if (end < 0) {
        end = fileSize - 1;
    }
    
    qint64 contentLength = end - start + 1;
    
    // Read file data from torrent
    // Note: This is a simplified implementation. In production, you'd want
    // to use libtorrent's piece picker to prioritize pieces for streaming
    QByteArray data;
    
    // For now, return a simple response indicating the file
    // In a full implementation, you'd stream the actual file data
    QString fileName = QString::fromStdString(files.file_path(ltFileIndex));
    QMimeDatabase mimeDb;
    QMimeType mimeType = mimeDb.mimeTypeForFile(fileName);

    // Map file range to pieces
    libtorrent::peer_request pr = ti.map_file(ltFileIndex, start, contentLength);
    libtorrent::piece_index_t piece_index = pr.piece;
    
    // Prioritize pieces for streaming - set deadline to 0 (highest priority)
    handle.set_piece_deadline(piece_index, 0, libtorrent::torrent_handle::alert_when_available);
    
    // Wait for the piece to be available
    int maxWaitIterations = 100; // 10 seconds max wait
    int iterations = 0;
    while (!handle.have_piece(piece_index) && iterations < maxWaitIterations) {
        QThread::msleep(100);
        iterations++;
        // Process alerts to ensure read_piece_alert is handled
        processTorrentAlerts();
    }
    
    if (!handle.have_piece(piece_index)) {
        return QHttpServerResponse("Piece not available yet", 
            QHttpServerResponse::StatusCode::ServiceUnavailable);
    }
    
    // Read the piece asynchronously
    handle.read_piece(piece_index);
    
    // Wait for read_piece_alert
    QByteArray pieceData;
    bool pieceRead = false;
    iterations = 0;
    while (!pieceRead && iterations < maxWaitIterations) {
        std::vector<libtorrent::alert*> alerts;
        m_session.pop_alerts(&alerts);
        
        for (libtorrent::alert* alert : alerts) {
            if (auto* rpa = libtorrent::alert_cast<libtorrent::read_piece_alert>(alert)) {
                if (rpa->piece == piece_index) {
                    if (rpa->error) {
                        return QHttpServerResponse("Error reading piece", 
                            QHttpServerResponse::StatusCode::InternalServerError);
                    }
                    // Extract data from alert buffer
                    const char* data = static_cast<const char*>(rpa->buffer.get());
                    int size = rpa->size;
                    pieceData = QByteArray(data, size);
                    pieceRead = true;
                    break;
                }
            }
        }
        
        if (!pieceRead) {
            QThread::msleep(100);
            iterations++;
        }
    }
    
    if (!pieceRead || pieceData.isEmpty()) {
        return QHttpServerResponse("Failed to read piece data", 
            QHttpServerResponse::StatusCode::InternalServerError);
    }
    
    // Calculate offset within the piece
    // The peer_request already contains the offset within the piece
    qint64 offsetInPiece = pr.start;
    
    // Extract the requested range from the piece data
    // Use the minimum of mapped length and contentLength to avoid over-reading
    int extractLength = qMin(static_cast<int>(pr.length), contentLength);
    QByteArray body = pieceData.mid(offsetInPiece, extractLength);
    
    // If the range spans multiple pieces, we'd need to read more pieces
    // For now, we handle single-piece ranges. Multi-piece ranges would require
    // reading multiple pieces and concatenating them.
    if (extractLength < contentLength) {
        // Partial data - this happens when range spans multiple pieces
        // For now, return what we have. A full implementation would read all pieces.
        LoggingService::logWarning("TorrentStreamServer", 
            QString("Range request spans multiple pieces, returning partial data"));
        // Adjust end to match actual data returned
        end = start + extractLength - 1;
        contentLength = extractLength;
    }
    
    #if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    // Newer API: use QHttpHeaders and setHeaders()
    QHttpHeaders headers;
    headers.append("Content-Type", mimeType.name().toUtf8());
    headers.append("Content-Length", QByteArray::number(contentLength));
    headers.append("Accept-Ranges", "bytes");
    headers.append("Content-Range", 
        QString("bytes %1-%2/%3").arg(start).arg(end).arg(fileSize).toUtf8());
    
    QHttpServerResponse response(body, QHttpServerResponse::StatusCode::PartialContent);
    response.setHeaders(headers);
    return response;
    #else
    // Older API: use setHeader() method
    QHttpServerResponse response(body, QHttpServerResponse::StatusCode::PartialContent);
    response.setHeader("Content-Type", mimeType.name().toUtf8());
    response.setHeader("Content-Length", QByteArray::number(contentLength));
    response.setHeader("Accept-Ranges", "bytes");
    response.setHeader("Content-Range", 
        QString("bytes %1-%2/%3").arg(start).arg(end).arg(fileSize).toUtf8());
    return response;
    #endif
}
#endif

QString TorrentStreamServer::generateStreamPath(const QString& torrentId, int fileIndex)
{
    if (fileIndex >= 0) {
        return QString("/stream/%1/%2").arg(torrentId, QString::number(fileIndex));
    }
    return QString("/stream/%1").arg(torrentId);
}

libtorrent::torrent_handle TorrentStreamServer::findTorrentByStreamUrl(const QString& streamUrl) const
{
    if (m_torrents.contains(streamUrl)) {
        return m_torrents.value(streamUrl).handle;
    }
    return libtorrent::torrent_handle();
}

QString TorrentStreamServer::getTorrentId(const QString& streamUrl) const
{
    if (m_torrents.contains(streamUrl)) {
        libtorrent::sha1_hash hash = m_torrents.value(streamUrl).handle.info_hash();
        // FIX: Replaced libtorrent::to_hex
        return QByteArray::fromStdString(hash.to_string()).toHex();
    }
    return QString();
}
#endif