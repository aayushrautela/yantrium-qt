#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QUrl>
#include <QCoreApplication>
#include <QDebug>
#include <QLoggingCategory>
#include <QTimer>
#include <iostream>
#include <QtQml>
#include <QQuickFramebufferObject>
#include "player/player_bridge.h"
#include "core/database/database_manager.h"
#include "core/di/service_registry.h"
#include "app_controller.h"
#include "features/addons/logic/addon_repository.h"
#include "core/services/tmdb_api_client.h"
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
#include "core/services/omdb_service.h"
#include <memory>

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

    qDebug() << "=== Yantrium Player Starting ===";
    
    // Force Qt to use OpenGL (Required for MDK video)
    qputenv("QSG_RHI_BACKEND", "opengl");
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    
    QGuiApplication app(argc, argv);

    // Initialize AppController
    AppController appController;
    if (!appController.initialize()) {
        qCritical() << "Failed to initialize application";
        return -1;
    }
    
    // Register services in service registry for dependency injection
    ServiceRegistry& registry = ServiceRegistry::instance();
    
    // Register core services as singletons
    qDebug() << "[MAIN] Registering TmdbDataService factory...";
    registry.registerSingleton<TmdbDataService>([]() {
        qDebug() << "[MAIN] TmdbDataService factory called";
        return std::make_shared<TmdbDataService>();
    });
    qDebug() << "[MAIN] TmdbDataService factory registered";
    
    qDebug() << "[MAIN] Registering TmdbSearchService factory...";
    registry.registerSingleton<TmdbSearchService>([]() {
        qDebug() << "[MAIN] TmdbSearchService factory called";
        return std::make_shared<TmdbSearchService>();
    });
    qDebug() << "[MAIN] TmdbSearchService factory registered";
    
    qDebug() << "[MAIN] Registering OmdbService factory...";
    registry.registerSingleton<OmdbService>([]() {
        qDebug() << "[MAIN] OmdbService factory called";
        return std::make_shared<OmdbService>();
    });
    qDebug() << "[MAIN] OmdbService factory registered";
    
    qDebug() << "[MAIN] Registering AddonRepository factory...";
    registry.registerSingleton<AddonRepository>([]() {
        qDebug() << "[MAIN] AddonRepository factory called";
        return std::make_shared<AddonRepository>();
    });
    qDebug() << "[MAIN] AddonRepository factory registered";
    
    qDebug() << "[MAIN] Registering LocalLibraryService factory...";
    registry.registerSingleton<LocalLibraryService>([]() {
        qDebug() << "[MAIN] LocalLibraryService factory called";
        return std::make_shared<LocalLibraryService>();
    });
    qDebug() << "[MAIN] LocalLibraryService factory registered";
    
    // Register MediaMetadataService (depends on TmdbDataService and OmdbService)
    qDebug() << "[MAIN] Registering MediaMetadataService factory...";
    registry.registerSingleton<MediaMetadataService>([&registry]() {
        qDebug() << "[MAIN] MediaMetadataService factory called";
        qDebug() << "[MAIN] Resolving dependencies in MediaMetadataService factory...";
        qDebug() << "[MAIN] Step 1: Resolving TmdbDataService...";
        std::shared_ptr<TmdbDataService> tmdbService;
        try {
            tmdbService = registry.resolve<TmdbDataService>();
            qDebug() << "[MAIN] TmdbDataService resolved in factory:" << (tmdbService ? "yes" : "no");
        } catch (const std::exception& e) {
            qCritical() << "[MAIN] Exception resolving TmdbDataService in factory:" << e.what();
            throw;
        } catch (...) {
            qCritical() << "[MAIN] Unknown exception resolving TmdbDataService in factory";
            throw;
        }
        qDebug() << "[MAIN] Step 2: Resolving OmdbService...";
        std::shared_ptr<OmdbService> omdbService;
        try {
            omdbService = registry.resolve<OmdbService>();
            qDebug() << "[MAIN] OmdbService resolved in factory:" << (omdbService ? "yes" : "no");
        } catch (const std::exception& e) {
            qCritical() << "[MAIN] Exception resolving OmdbService in factory:" << e.what();
            throw;
        } catch (...) {
            qCritical() << "[MAIN] Unknown exception resolving OmdbService in factory";
            throw;
        }
        qDebug() << "[MAIN] Step 3: Getting TraktCoreService instance...";
        TraktCoreService* traktService = nullptr;
        try {
            traktService = &TraktCoreService::instance();
            qDebug() << "[MAIN] TraktCoreService instance obtained:" << (traktService ? "yes" : "no");
        } catch (const std::exception& e) {
            qCritical() << "[MAIN] Exception getting TraktCoreService instance:" << e.what();
            throw;
        } catch (...) {
            qCritical() << "[MAIN] Unknown exception getting TraktCoreService instance";
            throw;
        }
        qDebug() << "[MAIN] Step 4: Creating MediaMetadataService instance...";
        try {
            auto service = std::make_shared<MediaMetadataService>(
                tmdbService, omdbService, traktService);
            qDebug() << "[MAIN] MediaMetadataService instance created successfully";
            return service;
        } catch (const std::exception& e) {
            qCritical() << "[MAIN] Exception creating MediaMetadataService:" << e.what();
            throw;
        } catch (...) {
            qCritical() << "[MAIN] Unknown exception creating MediaMetadataService";
            throw;
        }
    });
    qDebug() << "[MAIN] MediaMetadataService factory registered";
    
    // Register StreamService (depends on AddonRepository and TmdbDataService)
    registry.registerSingleton<StreamService>([&registry]() {
        auto addonRepo = registry.resolve<AddonRepository>();
        auto tmdbService = registry.resolve<TmdbDataService>();
        return std::make_shared<StreamService>(addonRepo, tmdbService, nullptr);
    });
    
    // Register LibraryService (depends on multiple services)
    registry.registerSingleton<LibraryService>([&registry]() {
        auto addonRepo = registry.resolve<AddonRepository>();
        auto tmdbService = registry.resolve<TmdbDataService>();
        auto tmdbSearchService = registry.resolve<TmdbSearchService>();
        auto mediaMetadataService = registry.resolve<MediaMetadataService>();
        auto omdbService = registry.resolve<OmdbService>();
        auto localLibraryService = registry.resolve<LocalLibraryService>();
        auto catalogDao = std::make_unique<CatalogPreferencesDao>();
        return std::make_shared<LibraryService>(
            addonRepo, tmdbService, tmdbSearchService, mediaMetadataService,
            omdbService, localLibraryService, std::move(catalogDao),
            &TraktCoreService::instance());
    });
    
    qDebug() << "[MAIN] Services registered in service registry";
    qDebug() << "[MAIN] About to register QML types...";

    // Register player component
    qDebug() << "[MAIN] Registering PlayerBridge...";
    qmlRegisterType<PlayerBridge>("Yantrium.Components", 1, 0, "VideoPlayer");
    qDebug() << "[MAIN] PlayerBridge registered";

    // Register services for QML from registry
    qDebug() << "[MAIN] Resolving services from registry...";
    
    qDebug() << "[MAIN] Resolving AddonRepository...";
    std::shared_ptr<AddonRepository> addonRepo;
    try {
        addonRepo = registry.resolve<AddonRepository>();
        qDebug() << "[MAIN] AddonRepository resolved:" << (addonRepo ? "success" : "failed");
    } catch (const std::exception& e) {
        qCritical() << "[MAIN] Exception resolving AddonRepository:" << e.what();
        return -1;
    } catch (...) {
        qCritical() << "[MAIN] Unknown exception resolving AddonRepository";
        return -1;
    }
    
    qDebug() << "[MAIN] Resolving TmdbDataService...";
    std::shared_ptr<TmdbDataService> tmdbService;
    try {
        tmdbService = registry.resolve<TmdbDataService>();
        qDebug() << "[MAIN] TmdbDataService resolved:" << (tmdbService ? "success" : "failed");
    } catch (const std::exception& e) {
        qCritical() << "[MAIN] Exception resolving TmdbDataService:" << e.what();
        return -1;
    } catch (...) {
        qCritical() << "[MAIN] Unknown exception resolving TmdbDataService";
        return -1;
    }
    
    qDebug() << "[MAIN] Resolving MediaMetadataService...";
    std::shared_ptr<MediaMetadataService> mediaMetadataService;
    try {
        mediaMetadataService = registry.resolve<MediaMetadataService>();
        qDebug() << "[MAIN] MediaMetadataService resolved:" << (mediaMetadataService ? "success" : "failed");
    } catch (const std::exception& e) {
        qCritical() << "[MAIN] Exception resolving MediaMetadataService:" << e.what();
        return -1;
    } catch (...) {
        qCritical() << "[MAIN] Unknown exception resolving MediaMetadataService";
        return -1;
    }
    
    qDebug() << "[MAIN] Services resolved";
    
    if (addonRepo) {
        qDebug() << "[MAIN] Registering AddonRepository...";
        qmlRegisterSingletonInstance("Yantrium.Components", 1, 0, "AddonRepository", addonRepo.get());
        qDebug() << "[MAIN] AddonRepository registered";
    }
    if (tmdbService) {
        qDebug() << "[MAIN] Registering TmdbDataService...";
        qmlRegisterSingletonInstance("Yantrium.Services", 1, 0, "TmdbDataService", tmdbService.get());
        qDebug() << "[MAIN] TmdbDataService registered";
    }
    if (mediaMetadataService) {
        qDebug() << "[MAIN] Registering MediaMetadataService...";
        qmlRegisterSingletonInstance("Yantrium.Services", 1, 0, "MediaMetadataService", mediaMetadataService.get());
        qDebug() << "[MAIN] MediaMetadataService registered";
    }
    
    // Register Trakt services (singletons)
    qDebug() << "[MAIN] Registering Trakt services...";
    qmlRegisterSingletonInstance("Yantrium.Services", 1, 0, "TraktAuthService", &TraktAuthService::instance());
    qDebug() << "[MAIN] TraktAuthService registered";
    qmlRegisterSingletonInstance("Yantrium.Services", 1, 0, "TraktCoreService", &TraktCoreService::instance());
    qDebug() << "[MAIN] TraktCoreService registered";
    qmlRegisterType<TraktScrobbleService>("Yantrium.Services", 1, 0, "TraktScrobbleService");
    qDebug() << "[MAIN] TraktScrobbleService registered";
    qmlRegisterType<TraktWatchlistService>("Yantrium.Services", 1, 0, "TraktWatchlistService");
    qDebug() << "[MAIN] TraktWatchlistService registered";
    
    // Register CatalogPreferencesService (depends on AddonRepository)
    qDebug() << "[MAIN] Registering CatalogPreferencesService in registry...";
    registry.registerSingleton<CatalogPreferencesService>([&registry]() {
        auto addonRepo = registry.resolve<AddonRepository>();
        auto dao = std::make_unique<CatalogPreferencesDao>();
        return std::make_shared<CatalogPreferencesService>(std::move(dao), addonRepo);
    });
    qDebug() << "[MAIN] CatalogPreferencesService registered in registry";
    
    // Register all services for QML from registry
    qDebug() << "[MAIN] Resolving additional services from registry...";
    auto libraryService = registry.resolve<LibraryService>();
    auto catalogPrefsService = registry.resolve<CatalogPreferencesService>();
    auto tmdbSearchService = registry.resolve<TmdbSearchService>();
    auto streamService = registry.resolve<StreamService>();
    auto localLibraryService = registry.resolve<LocalLibraryService>();
    qDebug() << "[MAIN] Additional services resolved";
    
    if (libraryService) {
        qDebug() << "[MAIN] Registering LibraryService...";
        qmlRegisterSingletonInstance("Yantrium.Services", 1, 0, "LibraryService", libraryService.get());
        qDebug() << "[MAIN] LibraryService registered";
    }
    if (catalogPrefsService) {
        qDebug() << "[MAIN] Registering CatalogPreferencesService...";
        qmlRegisterSingletonInstance("Yantrium.Services", 1, 0, "CatalogPreferencesService", catalogPrefsService.get());
        qDebug() << "[MAIN] CatalogPreferencesService registered";
    }
    if (tmdbSearchService) {
        qDebug() << "[MAIN] Registering TmdbSearchService...";
        qmlRegisterSingletonInstance("Yantrium.Services", 1, 0, "TmdbSearchService", tmdbSearchService.get());
        qDebug() << "[MAIN] TmdbSearchService registered";
    }
    if (streamService) {
        qDebug() << "[MAIN] Registering StreamService...";
        qmlRegisterSingletonInstance("Yantrium.Services", 1, 0, "StreamService", streamService.get());
        qDebug() << "[MAIN] StreamService registered";
    }
    if (localLibraryService) {
        qDebug() << "[MAIN] Registering LocalLibraryService...";
        qmlRegisterSingletonInstance("Yantrium.Services", 1, 0, "LocalLibraryService", localLibraryService.get());
        qDebug() << "[MAIN] LocalLibraryService registered";
    }
    
    // Register File Export service (no dependencies, can be instantiated directly)
    qDebug() << "[MAIN] Registering FileExportService...";
    qmlRegisterType<FileExportService>("Yantrium.Services", 1, 0, "FileExportService");
    qDebug() << "[MAIN] FileExportService registered";
    
    qDebug() << "[MAIN] All QML types registered";
    qDebug() << "[MAIN] Creating QML engine...";

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/qml/MainApp.qml"));
    
    // Connect to QML errors
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, [](const QList<QQmlError> &warnings) {
        for (const auto &warning : warnings) {
            qWarning() << "[QML Warning]" << warning.toString();
        }
    });
    
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            qCritical() << "[MAIN] Failed to load QML object from" << objUrl;
            QCoreApplication::exit(-1);
        } else if (obj) {
            qDebug() << "[MAIN] QML object created successfully from" << objUrl;
            // Try to get the root window and ensure it's visible
            if (auto *window = qobject_cast<QQuickWindow*>(obj)) {
                qDebug() << "[MAIN] Root window found, visible:" << window->isVisible();
                qDebug() << "[MAIN] Window size:" << window->width() << "x" << window->height();
                window->show();
                qDebug() << "[MAIN] Window shown, visible:" << window->isVisible();
            } else if (auto *item = qobject_cast<QQuickItem*>(obj)) {
                qDebug() << "[MAIN] Root item found";
                if (auto *window = item->window()) {
                    qDebug() << "[MAIN] Item window found, visible:" << window->isVisible();
                    window->show();
                }
            }
        }
    }, Qt::QueuedConnection);
    
    qDebug() << "[MAIN] Loading QML from:" << url;
    engine.load(url);
    qDebug() << "[MAIN] QML load called";
    
    // Check root objects after a short delay to ensure QML has processed
    QTimer::singleShot(100, [&engine]() {
        auto rootObjects = engine.rootObjects();
        qDebug() << "[MAIN] Root objects count:" << rootObjects.size();
        for (auto *obj : rootObjects) {
            qDebug() << "[MAIN] Root object type:" << obj->metaObject()->className();
            if (auto *window = qobject_cast<QQuickWindow*>(obj)) {
                qDebug() << "[MAIN] Found ApplicationWindow, visible:" << window->isVisible();
                qDebug() << "[MAIN] Window size:" << window->width() << "x" << window->height();
                if (!window->isVisible()) {
                    qWarning() << "[MAIN] Window is not visible, forcing show()";
                    window->show();
                }
            }
        }
    });
    
    qDebug() << "[MAIN] Entering event loop";

    return app.exec();
}

