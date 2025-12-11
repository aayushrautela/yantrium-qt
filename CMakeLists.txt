cmake_minimum_required(VERSION 3.16)

project(Yantrium VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Modern CMake: use target-based approach
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /WX-)
    add_compile_definitions(_WIN32_WINNT=0x0601)  # Windows 7 minimum
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# 1. ENABLE QT AUTOMATION
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 2. Find Qt6 Packages
find_package(Qt6 REQUIRED COMPONENTS Quick Widgets Network Gui Sql Core5Compat Svg)

# HttpServer is optional (only needed for torrent streaming)
find_package(Qt6HttpServer QUIET)
if(Qt6HttpServer_FOUND)
    message(STATUS "Qt6HttpServer found - torrent streaming enabled")
    add_compile_definitions(HAVE_QHTTPSERVER)
else()
    message(STATUS "Qt6HttpServer not found - torrent streaming disabled")
endif()

# Added: PkgConfig is required for robust Linux/Flatpak builds
find_package(PkgConfig QUIET)

# 3. Add Source Files
set(PROJECT_SOURCES
    src/main.cpp
    src/app_controller.cpp
    src/app_controller.h
    # Core DI
    src/core/di/service_registry.cpp
    src/core/di/service_registry.h
    # Core service interfaces
    src/core/services/interfaces/imedia_metadata_service.h
    src/core/services/interfaces/istream_service.h
    src/core/services/interfaces/ilibrary_service.h
    # Core concepts and errors
    src/core/concepts.h
    src/core/errors/result.h
    src/core/errors/error_types.h
    src/player/player_bridge.cpp
    src/player/player_bridge.h
    src/player/mdk_player.cpp
    src/player/mdk_player.h
    # Core database
    src/core/database/database_manager.cpp
    src/core/database/database_manager.h
    src/core/database/addon_dao.cpp
    src/core/database/addon_dao.h
    src/core/database/trakt_auth_dao.cpp
    src/core/database/trakt_auth_dao.h
    src/core/database/catalog_preferences_dao.cpp
    src/core/database/catalog_preferences_dao.h
    src/core/database/local_library_dao.cpp
    src/core/database/local_library_dao.h
    src/core/database/watch_history_dao.cpp
    src/core/database/watch_history_dao.h
    src/core/database/sync_tracking_dao.cpp
    src/core/database/sync_tracking_dao.h
    # Addon models
    src/features/addons/models/addon_manifest.cpp
    src/features/addons/models/addon_manifest.h
    src/features/addons/models/catalog_definition.cpp
    src/features/addons/models/catalog_definition.h
    src/features/addons/models/addon_config.cpp
    src/features/addons/models/addon_config.h
    # Addon logic
    src/features/addons/logic/addon_client.cpp
    src/features/addons/logic/addon_client.h
    src/features/addons/logic/addon_installer.cpp
    src/features/addons/logic/addon_installer.h
    src/features/addons/logic/addon_repository.cpp
    src/features/addons/logic/addon_repository.h
    # Core models
    src/core/models/trakt_models.cpp
    src/core/models/trakt_models.h
    src/core/models/stream_info.cpp
    src/core/models/stream_info.h
    # Core services
    src/core/services/configuration.cpp
    src/core/services/configuration.h
    src/core/services/id_parser.h
    src/core/services/frontend_data_mapper.cpp
    src/core/services/frontend_data_mapper.h
    src/core/services/media_metadata_service.cpp
    src/core/services/media_metadata_service.h
    src/core/services/omdb_service.cpp
    src/core/services/omdb_service.h
    src/core/services/trakt_core_service.cpp
    src/core/services/trakt_core_service.h
    src/core/services/trakt_cache_helper.cpp
    src/core/services/trakt_cache_helper.h
    src/core/services/trakt_auth_service.cpp
    src/core/services/trakt_auth_service.h
    src/core/services/trakt_scrobble_service.cpp
    src/core/services/trakt_scrobble_service.h
    src/core/services/trakt_watchlist_service.cpp
    src/core/services/trakt_watchlist_service.h
    src/core/services/library_service.cpp
    src/core/services/library_service.h
    src/core/services/catalog_preferences_service.cpp
    src/core/services/catalog_preferences_service.h
    src/core/services/file_export_service.cpp
    src/core/services/file_export_service.h
    src/core/services/local_library_service.cpp
    src/core/services/local_library_service.h
    src/core/services/stream_service.cpp
    src/core/services/stream_service.h
    src/core/services/navigation_service.cpp
    src/core/services/navigation_service.h
    src/core/services/logging_service.cpp
    src/core/services/logging_service.h
    src/core/services/cache_service.cpp
    src/core/services/cache_service.h
    src/core/services/torrent_service.cpp
    src/core/services/torrent_service.h
    src/core/services/torrent_stream_server.cpp
    src/core/services/torrent_stream_server.h
    resources/qml.qrc
)

# 4. Define the Executable
qt_add_executable(Yantrium
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

# Add src directory to include path
target_include_directories(Yantrium PRIVATE src)

# --- API KEYS ---

option(TRAKT_CLIENT_ID "Trakt Client ID" "")
if(TRAKT_CLIENT_ID)
    target_compile_definitions(Yantrium PRIVATE TRAKT_CLIENT_ID="${TRAKT_CLIENT_ID}")
else()
    target_compile_definitions(Yantrium PRIVATE TRAKT_CLIENT_ID="")
endif()

option(TRAKT_CLIENT_SECRET "Trakt Client Secret" "")
if(TRAKT_CLIENT_SECRET)
    target_compile_definitions(Yantrium PRIVATE TRAKT_CLIENT_SECRET="${TRAKT_CLIENT_SECRET}")
else()
    target_compile_definitions(Yantrium PRIVATE TRAKT_CLIENT_SECRET="")
endif()

option(OMDB_API_KEY "OMDB API Key" "")
if(OMDB_API_KEY)
    target_compile_definitions(Yantrium PRIVATE OMDB_API_KEY="${OMDB_API_KEY}")
else()
    target_compile_definitions(Yantrium PRIVATE OMDB_API_KEY="")
endif()

# 5. Link Qt Libraries
target_link_libraries(Yantrium PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::Widgets
    Qt6::Network
    Qt6::Gui
    Qt6::Sql
    Qt6::Core5Compat
    Qt6::Svg
)

# Link HttpServer only if available
if(Qt6HttpServer_FOUND)
    target_link_libraries(Yantrium PRIVATE Qt6::HttpServer)
endif()

# =========================================================
# 6. PLATFORM SPECIFIC CONFIGURATION
# =========================================================

if(WIN32)
    include(cmake/WindowsConfig.cmake)
elseif(UNIX AND NOT APPLE)
    include(cmake/LinuxConfig.cmake)
endif()

qt_import_qml_plugins(Yantrium)

# Install rules
install(TARGETS Yantrium
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# --- CRITICAL ---
# Finalize the target (Required because MANUAL_FINALIZATION was used)
qt_finalize_executable(Yantrium)