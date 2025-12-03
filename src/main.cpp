#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QUrl>
#include <QCoreApplication>
#include <QDebug>
#include <QLoggingCategory>
#include <iostream>
#include <QtQml>
#include <QQuickFramebufferObject>
#include "player/player_bridge.h"
#include "core/database/database_manager.h"
#include "features/addons/logic/addon_repository.h"
#include "core/services/tmdb_data_service.h"
#include "core/services/media_metadata_service.h"
#include "core/services/trakt_auth_service.h"
#include "core/services/trakt_core_service.h"
#include "core/services/trakt_scrobble_service.h"
#include "core/services/trakt_watchlist_service.h"
#include "core/services/library_service.h"
#include "core/services/catalog_preferences_service.h"
#include "core/services/file_export_service.h"
#include "core/services/local_library_service.h"
#include "core/services/tmdb_search_service.h"
#include "core/services/stream_service.h"

// Force logging to console
Q_LOGGING_CATEGORY(yantrium, "yantrium")

int main(int argc, char *argv[])
{
    // MUST BE FIRST - Tells Qt to use OpenGL instead of Vulkan/Metal
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    
    // Force unbuffered output
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
    
    // Force console output
    qSetMessagePattern("[%{time yyyy-MM-dd hh:mm:ss.zzz}] [%{category}] %{message}");

    // Enable QML console.log messages (Qt6 filters them by default)
    qputenv("QT_LOGGING_RULES", "qml.debug=true;js.debug=true");

#ifdef Q_OS_WIN
    // Add Qt plugin paths for Windows deployment
    // This ensures plugins can be found even if not in standard locations
    QString appDir = QCoreApplication::applicationDirPath();
    QCoreApplication::addLibraryPath(appDir + "/plugins");
    QCoreApplication::addLibraryPath(appDir);
    qDebug() << "[MAIN] Windows: Added plugin paths - appDir:" << appDir;
#endif

    std::cout << "=== Yantrium Player Starting ===" << std::endl;
    std::cerr << "=== Yantrium Player Starting (stderr) ===" << std::endl;
    std::cout.flush();
    std::cerr.flush();
    qDebug() << "=== Yantrium Player Starting ===";
    
    // CRITICAL FIX: Force Qt to use OpenGL (Required for MDK video)
    // Fedora uses Wayland/Vulkan by default, which breaks MDK rendering.
    // This MUST be set BEFORE QGuiApplication is created
    qputenv("QSG_RHI_BACKEND", "opengl");
    std::cout << "[MAIN] Set QSG_RHI_BACKEND to opengl" << std::endl;
    std::cout << "[MAIN] QSG_RHI_BACKEND env var: " << qgetenv("QSG_RHI_BACKEND").constData() << std::endl;
    std::cout.flush();
    qDebug() << "Set QSG_RHI_BACKEND to opengl";
    qDebug() << "QSG_RHI_BACKEND env var:" << qgetenv("QSG_RHI_BACKEND");
    
    // Set Qt Quick Controls style to Basic (minimal style that supports background customization)
    // Native Windows style doesn't support background property customization
    // This must be set BEFORE QGuiApplication is created
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    std::cout << "[MAIN] Set QT_QUICK_CONTROLS_STYLE to Basic" << std::endl;
    std::cout.flush();
    qDebug() << "QT_QUICK_CONTROLS_STYLE set to Basic";
    
    QGuiApplication app(argc, argv);
    std::cout << "[MAIN] QGuiApplication created" << std::endl;
    std::cout.flush();
    qDebug() << "QGuiApplication created";

    // Initialize database
    DatabaseManager& dbManager = DatabaseManager::instance();
    if (!dbManager.initialize()) {
        std::cerr << "[MAIN] ERROR: Failed to initialize database" << std::endl;
        std::cerr.flush();
        qCritical() << "Failed to initialize database";
        return -1;
    }
    std::cout << "[MAIN] Database initialized successfully" << std::endl;
    std::cout.flush();
    qDebug() << "Database initialized successfully";

    // Register our player
    qmlRegisterType<PlayerBridge>("Yantrium.Components", 1, 0, "VideoPlayer");
    std::cout << "[MAIN] Registered PlayerBridge as VideoPlayer" << std::endl;
    std::cout.flush();
    qDebug() << "Registered PlayerBridge as VideoPlayer";

    // Register addon repository
    qmlRegisterType<AddonRepository>("Yantrium.Components", 1, 0, "AddonRepository");
    std::cout << "[MAIN] Registered AddonRepository" << std::endl;
    std::cout.flush();
    qDebug() << "Registered AddonRepository";

    // Register TMDB services
    qmlRegisterType<TmdbDataService>("Yantrium.Services", 1, 0, "TmdbDataService");
    qmlRegisterType<MediaMetadataService>("Yantrium.Services", 1, 0, "MediaMetadataService");
    std::cout << "[MAIN] Registered TMDB services" << std::endl;
    std::cout.flush();
    qDebug() << "Registered TMDB services";
    
    // Register Trakt services
    // Register TraktAuthService as singleton so all QML components share the same instance
    qmlRegisterSingletonInstance("Yantrium.Services", 1, 0, "TraktAuthService", &TraktAuthService::instance());
    // Register TraktCoreService as singleton so all QML components share the same instance
    qmlRegisterSingletonInstance("Yantrium.Services", 1, 0, "TraktCoreService", &TraktCoreService::instance());
    qmlRegisterType<TraktScrobbleService>("Yantrium.Services", 1, 0, "TraktScrobbleService");
    qmlRegisterType<TraktWatchlistService>("Yantrium.Services", 1, 0, "TraktWatchlistService");
    std::cout << "[MAIN] Registered Trakt services" << std::endl;
    std::cout.flush();
    qDebug() << "Registered Trakt services";
    
    // Register Library service
    qmlRegisterType<LibraryService>("Yantrium.Services", 1, 0, "LibraryService");
    std::cout << "[MAIN] Registered LibraryService" << std::endl;
    std::cout.flush();
    qDebug() << "Registered LibraryService";
    
    // Register Catalog Preferences service
    qmlRegisterType<CatalogPreferencesService>("Yantrium.Services", 1, 0, "CatalogPreferencesService");
    std::cout << "[MAIN] Registered CatalogPreferencesService" << std::endl;
    std::cout.flush();
    qDebug() << "Registered CatalogPreferencesService";
    
    // Register File Export service
    qmlRegisterType<FileExportService>("Yantrium.Services", 1, 0, "FileExportService");
    std::cout << "[MAIN] Registered FileExportService" << std::endl;
    std::cout.flush();
    qDebug() << "Registered FileExportService";
    
    // Register Local Library service
    qmlRegisterType<LocalLibraryService>("Yantrium.Services", 1, 0, "LocalLibraryService");
    std::cout << "[MAIN] Registered LocalLibraryService" << std::endl;
    std::cout.flush();
    qDebug() << "Registered LocalLibraryService";
    
    // Register TMDB Search service
    qmlRegisterType<TmdbSearchService>("Yantrium.Services", 1, 0, "TmdbSearchService");
    std::cout << "[MAIN] Registered TmdbSearchService" << std::endl;
    std::cout.flush();
    qDebug() << "Registered TmdbSearchService";
    
    // Register Stream service
    qmlRegisterType<StreamService>("Yantrium.Services", 1, 0, "StreamService");
    std::cout << "[MAIN] Registered StreamService" << std::endl;
    std::cout.flush();
    qDebug() << "Registered StreamService";

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/qml/MainApp.qml"));
    std::cout << "[MAIN] Loading QML from: " << url.toString().toStdString() << std::endl;
    std::cout.flush();
    qDebug() << "Loading QML from:" << url;
    
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            std::cout << "[MAIN] ERROR: Failed to load QML object from " << objUrl.toString().toStdString() << std::endl;
            std::cout.flush();
            qDebug() << "ERROR: Failed to load QML object from" << objUrl;
            QCoreApplication::exit(-1);
        } else if (obj) {
            std::cout << "[MAIN] QML object created successfully from " << objUrl.toString().toStdString() << std::endl;
            std::cout.flush();
            qDebug() << "QML object created successfully from" << objUrl;
        }
    }, Qt::QueuedConnection);
    
    engine.load(url);
    std::cout << "[MAIN] QML engine loaded, entering event loop" << std::endl;
    std::cout.flush();
    qDebug() << "QML engine loaded, entering event loop";

    return app.exec();
}

