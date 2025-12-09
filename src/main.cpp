#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QUrl>
#include <QCoreApplication>
#include <QDebug>
#include <QLoggingCategory>
#include <QTimer>
#include <QImageReader>
#include <iostream>
#include <QtQml>
#include <QQuickFramebufferObject>
#include "player/player_bridge.h"
#include "core/database/database_manager.h"
#include "core/services/configuration.h"
#include "core/di/service_registry.h"
#include "app_controller.h"
#include "features/addons/logic/addon_repository.h"
#include "core/services/media_metadata_service.h"
#include "core/services/trakt_auth_service.h"
#include "core/services/trakt_core_service.h"
#include "core/services/trakt_scrobble_service.h"
#include "core/services/trakt_watchlist_service.h"
#include "core/services/library_service.h"
#include "core/services/catalog_preferences_service.h"
#include "core/services/file_export_service.h"
#include "core/services/local_library_service.h"
#include "core/services/stream_service.h"
#include "core/services/omdb_service.h"
#include "core/services/navigation_service.h"
#include "core/services/logging_service.h"
#include "core/services/cache_service.h"
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
    
    // Check for SVG support (critical for rating logos, especially in Flatpak)
    QList<QByteArray> supportedFormats = QImageReader::supportedImageFormats();
    bool hasSvg = supportedFormats.contains("svg") || supportedFormats.contains("svgz");
    if (!hasSvg) {
        qWarning() << "[MAIN] WARNING: SVG format not supported! Rating logos may not display.";
        qWarning() << "[MAIN] Available formats:" << supportedFormats;
        qWarning() << "[MAIN] This is often a Flatpak/plugin issue. Ensure Qt SVG plugin is available.";
    } else {
        qDebug() << "[MAIN] SVG format is supported";
    }
    
    // Force Qt to use OpenGL (Required for MDK video)
    qputenv("QSG_RHI_BACKEND", "opengl");
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    
    QGuiApplication app(argc, argv);

    // Register services in service registry for dependency injection FIRST
    // This must happen before AppController::initialize() which needs DatabaseManager
    ServiceRegistry& registry = ServiceRegistry::instance();
    
    // Register infrastructure services first (before other services that depend on them)
    qDebug() << "[MAIN] Registering Configuration factory...";
    registry.registerSingleton<Configuration>([]() {
        qDebug() << "[MAIN] Configuration factory called";
        return std::make_shared<Configuration>();
    });
    qDebug() << "[MAIN] Configuration factory registered";
    
    qDebug() << "[MAIN] Registering DatabaseManager factory...";
    registry.registerSingleton<DatabaseManager>([]() {
        qDebug() << "[MAIN] DatabaseManager factory called";
        auto dbManager = std::make_shared<DatabaseManager>();
        // Initialize database immediately after creation
        if (!dbManager->initialize()) {
            qCritical() << "[MAIN] Failed to initialize database in factory";
        }
        return dbManager;
    });
    qDebug() << "[MAIN] DatabaseManager factory registered";
    
    // Initialize AppController (now that DatabaseManager is registered)
    AppController appController;
    if (!appController.initialize()) {
        qCritical() << "Failed to initialize application";
        return -1;
    }
    
    // Register core utility services (LoggingService includes error reporting, CacheService, NavigationService)
    qDebug() << "[MAIN] Registering LoggingService factory...";
    registry.registerSingleton<LoggingService>([]() {
        qDebug() << "[MAIN] LoggingService factory called";
        return std::make_shared<LoggingService>();
    });
    qDebug() << "[MAIN] LoggingService factory registered";
    
    qDebug() << "[MAIN] Registering CacheService factory...";
    registry.registerSingleton<CacheService>([]() {
        qDebug() << "[MAIN] CacheService factory called";
        return std::make_shared<CacheService>();
    });
    qDebug() << "[MAIN] CacheService factory registered";
    
    qDebug() << "[MAIN] Registering NavigationService factory...";
    registry.registerSingleton<NavigationService>([]() {
        qDebug() << "[MAIN] NavigationService factory called";
        return std::make_shared<NavigationService>();
    });
    qDebug() << "[MAIN] NavigationService factory registered";
    
    // Register core services as singletons
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
    
    // Register MediaMetadataService (depends on OmdbService and AddonRepository)
    qDebug() << "[MAIN] Registering MediaMetadataService factory...";
    registry.registerSingleton<MediaMetadataService>([&registry]() {
        qDebug() << "[MAIN] MediaMetadataService factory called";
        qDebug() << "[MAIN] Resolving dependencies in MediaMetadataService factory...";
        qDebug() << "[MAIN] Step 1: Resolving OmdbService...";
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
        qDebug() << "[MAIN] Step 2: Resolving AddonRepository...";
        std::shared_ptr<AddonRepository> addonRepo;
        try {
            addonRepo = registry.resolve<AddonRepository>();
            qDebug() << "[MAIN] AddonRepository resolved in factory:" << (addonRepo ? "yes" : "no");
        } catch (const std::exception& e) {
            qCritical() << "[MAIN] Exception resolving AddonRepository in factory:" << e.what();
            throw;
        } catch (...) {
            qCritical() << "[MAIN] Unknown exception resolving AddonRepository in factory";
            throw;
        }
        qDebug() << "[MAIN] Step 3: Resolving TraktCoreService from registry...";
        std::shared_ptr<TraktCoreService> traktService = nullptr;
        try {
            traktService = registry.resolve<TraktCoreService>();
            qDebug() << "[MAIN] TraktCoreService resolved in factory:" << (traktService ? "yes" : "no");
        } catch (const std::exception& e) {
            qCritical() << "[MAIN] Exception resolving TraktCoreService in factory:" << e.what();
            throw;
        } catch (...) {
            qCritical() << "[MAIN] Unknown exception resolving TraktCoreService in factory";
            throw;
        }
        qDebug() << "[MAIN] Step 4: Creating MediaMetadataService instance...";
        try {
            auto service = std::make_shared<MediaMetadataService>(
                omdbService, addonRepo, traktService ? traktService.get() : nullptr);
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
    
    // Register StreamService (depends on AddonRepository)
    registry.registerSingleton<StreamService>([&registry]() {
        auto addonRepo = registry.resolve<AddonRepository>();
        return std::make_shared<StreamService>(addonRepo, nullptr);
    });
    
    // Register LibraryService (depends on multiple services)
    registry.registerSingleton<LibraryService>([&registry]() {
        auto addonRepo = registry.resolve<AddonRepository>();
        auto mediaMetadataService = registry.resolve<MediaMetadataService>();
        auto omdbService = registry.resolve<OmdbService>();
        auto localLibraryService = registry.resolve<LocalLibraryService>();
        auto catalogDao = std::make_unique<CatalogPreferencesDao>();
        auto traktService = registry.resolve<TraktCoreService>();
        return std::make_shared<LibraryService>(
            addonRepo, mediaMetadataService,
            omdbService, localLibraryService, std::move(catalogDao),
            traktService ? traktService.get() : nullptr);
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
    if (mediaMetadataService) {
        qDebug() << "[MAIN] Registering MediaMetadataService...";
        qmlRegisterSingletonInstance("Yantrium.Services", 1, 0, "MediaMetadataService", mediaMetadataService.get());
        qDebug() << "[MAIN] MediaMetadataService registered";
    }
    
    // Register Trakt services (register TraktCoreService FIRST so it's available when TraktAuthService is created)
    qDebug() << "[MAIN] Registering Trakt services...";
    
    // Register TraktCoreService FIRST
    qDebug() << "[MAIN] Registering TraktCoreService factory...";
    registry.registerSingleton<TraktCoreService>([]() {
        qDebug() << "[MAIN] TraktCoreService factory called";
        return std::make_shared<TraktCoreService>();
    });
    qDebug() << "[MAIN] TraktCoreService factory registered";
    
    auto traktCoreService = registry.resolve<TraktCoreService>();
    if (traktCoreService) {
        // Initialize TraktCoreService immediately so it's ready
        traktCoreService->initializeDatabase();
        traktCoreService->initializeAuth();
        qmlRegisterSingletonInstance("Yantrium.Services", 1, 0, "TraktCoreService", traktCoreService.get());
        qDebug() << "[MAIN] TraktCoreService registered for QML";
    }
    
    // Now register TraktAuthService (TraktCoreService is now available)
    registry.registerSingleton<TraktAuthService>([]() {
        qDebug() << "[MAIN] TraktAuthService factory called";
        return std::make_shared<TraktAuthService>();
    });
    qDebug() << "[MAIN] TraktAuthService factory registered";
    
    auto traktAuthService = registry.resolve<TraktAuthService>();
    if (traktAuthService) {
        // Check authentication now that both services are ready
        traktAuthService->checkAuthentication();
        qmlRegisterSingletonInstance("Yantrium.Services", 1, 0, "TraktAuthService", traktAuthService.get());
        qDebug() << "[MAIN] TraktAuthService registered for QML";
    }
    // Register TraktScrobbleService and TraktWatchlistService in registry
    qDebug() << "[MAIN] Registering TraktScrobbleService factory...";
    registry.registerSingleton<TraktScrobbleService>([]() {
        qDebug() << "[MAIN] TraktScrobbleService factory called";
        return std::make_shared<TraktScrobbleService>();
    });
    qDebug() << "[MAIN] TraktScrobbleService factory registered";
    
    qDebug() << "[MAIN] Registering TraktWatchlistService factory...";
    registry.registerSingleton<TraktWatchlistService>([]() {
        qDebug() << "[MAIN] TraktWatchlistService factory called";
        return std::make_shared<TraktWatchlistService>();
    });
    qDebug() << "[MAIN] TraktWatchlistService factory registered";
    
    auto traktScrobbleService = registry.resolve<TraktScrobbleService>();
    if (traktScrobbleService) {
        qmlRegisterSingletonInstance("Yantrium.Services", 1, 0, "TraktScrobbleService", traktScrobbleService.get());
        qDebug() << "[MAIN] TraktScrobbleService registered for QML";
    }
    
    auto traktWatchlistService = registry.resolve<TraktWatchlistService>();
    if (traktWatchlistService) {
        qmlRegisterSingletonInstance("Yantrium.Services", 1, 0, "TraktWatchlistService", traktWatchlistService.get());
        qDebug() << "[MAIN] TraktWatchlistService registered for QML";
    }
    
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
    
    // Register core utility services for QML (they have QML_SINGLETON so they work automatically, but register from registry for consistency)
    auto loggingService = registry.resolve<LoggingService>();
    if (loggingService) {
        qmlRegisterSingletonInstance("Yantrium.Services", 1, 0, "LoggingService", loggingService.get());
        qDebug() << "[MAIN] LoggingService registered for QML";
    }
    
    
    auto cacheService = registry.resolve<CacheService>();
    if (cacheService) {
        qmlRegisterSingletonInstance("Yantrium.Services", 1, 0, "CacheService", cacheService.get());
        qDebug() << "[MAIN] CacheService registered for QML";
    }
    
    auto navigationService = registry.resolve<NavigationService>();
    if (navigationService) {
        qmlRegisterSingletonInstance("Yantrium.Services", 1, 0, "NavigationService", navigationService.get());
        qDebug() << "[MAIN] NavigationService registered for QML";
    }
    
    // Register File Export service (no dependencies, can be instantiated directly)
    qDebug() << "[MAIN] Registering FileExportService...";
    qmlRegisterSingletonType<FileExportService>("Yantrium.Services", 1, 0, "FileExportService",
        [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject* {
            Q_UNUSED(engine)
            Q_UNUSED(scriptEngine)
            return new FileExportService();
        });
    qDebug() << "[MAIN] FileExportService registered";
    
    // Register NavigationService (includes screen management)
    qDebug() << "[MAIN] Registering NavigationService...";
    qmlRegisterSingletonType<NavigationService>("Yantrium.Services", 1, 0, "NavigationService",
        [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject* {
            Q_UNUSED(engine)
            Q_UNUSED(scriptEngine)
            return new NavigationService();
        });
    qDebug() << "[MAIN] NavigationService registered";
    
    // Register LoggingService
    qDebug() << "[MAIN] Registering LoggingService...";
    qmlRegisterSingletonType<LoggingService>("Yantrium.Services", 1, 0, "LoggingService",
        [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject* {
            Q_UNUSED(engine)
            Q_UNUSED(scriptEngine)
            return new LoggingService();
        });
    qDebug() << "[MAIN] LoggingService registered";
    
    // Register CacheService
    qDebug() << "[MAIN] Registering CacheService...";
    qmlRegisterSingletonType<CacheService>("Yantrium.Services", 1, 0, "CacheService",
        [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject* {
            Q_UNUSED(engine)
            Q_UNUSED(scriptEngine)
            return new CacheService();
        });
    qDebug() << "[MAIN] CacheService registered";
    
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
                if (auto *itemWindow = item->window()) {
                    qDebug() << "[MAIN] Item window found, visible:" << itemWindow->isVisible();
                    itemWindow->show();
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

