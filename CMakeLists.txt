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
# ADDED: 'Svg' (Fixes "Unsupported image format" for icons)
# CHANGED: 'Qt5Compat' -> 'Core5Compat' (Correct CMake name)
find_package(Qt6 REQUIRED COMPONENTS Quick Widgets Network Gui Sql Core5Compat Svg)

# HttpServer is optional (only needed for torrent streaming)
find_package(Qt6HttpServer QUIET)
if(Qt6HttpServer_FOUND)
    message(STATUS "Qt6HttpServer found - torrent streaming enabled")
    add_compile_definitions(HAVE_QHTTPSERVER)
else()
    message(STATUS "Qt6HttpServer not found - torrent streaming disabled")
endif()

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
# Note: MANUAL_FINALIZATION is used, so we must call qt_finalize_executable at the end
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
# ADDED: Qt6::Svg and Qt6::Core5Compat
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
# 6. MDK-SDK INTEGRATION
# =========================================================
set(MDK_DIR "${CMAKE_SOURCE_DIR}/vendor/mdk-sdk")

if(WIN32)
    target_include_directories(Yantrium PRIVATE ${MDK_DIR}/include)
    target_link_libraries(Yantrium PRIVATE ${MDK_DIR}/lib/x64/mdk.lib)
    
    # Copy MDK binaries to build directory on Windows
    add_custom_command(TARGET Yantrium POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${MDK_DIR}/bin/x64/ $<TARGET_FILE_DIR:Yantrium>)
        
elseif(UNIX AND NOT APPLE)
    target_include_directories(Yantrium PRIVATE ${MDK_DIR}/include)
    target_link_libraries(Yantrium PRIVATE ${MDK_DIR}/lib/amd64/libmdk.so)
    
    # Copy libmdk.so to build directory on Linux
    add_custom_command(TARGET Yantrium POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${MDK_DIR}/lib/amd64/libmdk.so $<TARGET_FILE_DIR:Yantrium>)
        
    set_target_properties(Yantrium PROPERTIES
        BUILD_RPATH "\$ORIGIN"
        INSTALL_RPATH "\$ORIGIN"
    )
endif()

# =========================================================
# 7. LIBTORRENT INTEGRATION
# =========================================================
set(LIBTORRENT_DIR "${CMAKE_SOURCE_DIR}/vendor/libtorrent")

if(WIN32)
    # Check if libtorrent is available in vendor directory
    if(EXISTS "${LIBTORRENT_DIR}/lib/x64/libtorrent-rasterbar.lib" OR 
       EXISTS "${LIBTORRENT_DIR}/lib/x64/torrent-rasterbar.lib")
        message(STATUS "Found bundled libtorrent for Windows")
        target_include_directories(Yantrium PRIVATE ${LIBTORRENT_DIR}/include)
        
        # Find the actual library file
        file(GLOB LIBTORRENT_LIBS "${LIBTORRENT_DIR}/lib/x64/*torrent*.lib")
        if(LIBTORRENT_LIBS)
            target_link_libraries(Yantrium PRIVATE ${LIBTORRENT_LIBS})
            target_compile_definitions(Yantrium PRIVATE TORRENT_SUPPORT_ENABLED)
            
            # Copy libtorrent DLLs to build directory
            add_custom_command(TARGET Yantrium POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${LIBTORRENT_DIR}/bin/x64/ $<TARGET_FILE_DIR:Yantrium>)
        endif()
    else()
        message(STATUS "libtorrent not found in vendor directory - torrent support disabled")
    endif()
    
elseif(UNIX AND NOT APPLE)
    # Check if libtorrent is available in vendor directory first
    file(GLOB LIBTORRENT_SO "${LIBTORRENT_DIR}/lib/amd64/libtorrent-rasterbar.so*")
    
    if(LIBTORRENT_SO)
        message(STATUS "Found bundled libtorrent for Linux in vendor directory")
        target_include_directories(Yantrium PRIVATE ${LIBTORRENT_DIR}/include)
        
        # Link against the main .so file (not versioned)
        find_file(LIBTORRENT_MAIN_SO 
            NAMES libtorrent-rasterbar.so
            PATHS ${LIBTORRENT_DIR}/lib/amd64
            NO_DEFAULT_PATH
        )
        if(LIBTORRENT_MAIN_SO)
            target_link_libraries(Yantrium PRIVATE ${LIBTORRENT_MAIN_SO})
            target_compile_definitions(Yantrium PRIVATE TORRENT_SUPPORT_ENABLED)
            
            # Copy all libtorrent .so files to build directory
            add_custom_command(TARGET Yantrium POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${LIBTORRENT_DIR}/lib/amd64/ $<TARGET_FILE_DIR:Yantrium>)
        else()
            # Fallback: link against first found .so file
            list(GET LIBTORRENT_SO 0 FIRST_SO)
            target_link_libraries(Yantrium PRIVATE ${FIRST_SO})
            target_compile_definitions(Yantrium PRIVATE TORRENT_SUPPORT_ENABLED)
            
            add_custom_command(TARGET Yantrium POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${LIBTORRENT_DIR}/lib/amd64/ $<TARGET_FILE_DIR:Yantrium>)
        endif()
        
        set_target_properties(Yantrium PROPERTIES
            BUILD_RPATH "\$ORIGIN"
            INSTALL_RPATH "\$ORIGIN"
        )
    else()
        # Try to find libtorrent from system/Flatpak paths (e.g., /app/lib)
        # First, check if /app/lib/libtorrent-rasterbar.so exists (Flatpak build)
        if(EXISTS "/app/lib/libtorrent-rasterbar.so")
            set(LIBTORRENT_LIB "/app/lib/libtorrent-rasterbar.so")
            message(STATUS "Found libtorrent at: ${LIBTORRENT_LIB}")
        else()
            # Try multiple library name variations
            find_library(LIBTORRENT_LIB 
                NAMES torrent-rasterbar libtorrent-rasterbar libtorrent
                PATHS /app/lib /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
                NO_DEFAULT_PATH
            )
            # Also try without NO_DEFAULT_PATH as fallback
            if(NOT LIBTORRENT_LIB)
                find_library(LIBTORRENT_LIB 
                    NAMES torrent-rasterbar libtorrent-rasterbar libtorrent
                )
            endif()
        endif()
        
        # Check for headers
        if(EXISTS "/app/include/libtorrent/session.hpp")
            set(LIBTORRENT_INCLUDE_DIR "/app/include")
            message(STATUS "Found libtorrent headers at: ${LIBTORRENT_INCLUDE_DIR}")
        else()
            find_path(LIBTORRENT_INCLUDE_DIR
                NAMES libtorrent/session.hpp
                PATHS /app/include /usr/include /usr/local/include
                NO_DEFAULT_PATH
            )
            # Also try without NO_DEFAULT_PATH as fallback
            if(NOT LIBTORRENT_INCLUDE_DIR)
                find_path(LIBTORRENT_INCLUDE_DIR
                    NAMES libtorrent/session.hpp
                )
            endif()
        endif()
        
        if(LIBTORRENT_LIB AND LIBTORRENT_INCLUDE_DIR)
            message(STATUS "Found libtorrent from system: ${LIBTORRENT_LIB}")
            message(STATUS "Found libtorrent headers: ${LIBTORRENT_INCLUDE_DIR}")
            target_include_directories(Yantrium PRIVATE ${LIBTORRENT_INCLUDE_DIR})
            target_compile_definitions(Yantrium PRIVATE TORRENT_SUPPORT_ENABLED)
            
            # Find and link boost libraries that libtorrent depends on
            # Boost is built in the Flatpak manifest, so it should be in /app/lib
            # Check for boost libraries in /app/lib first (Flatpak build)
            if(EXISTS "/app/lib/libboost_system.so")
                set(BOOST_SYSTEM_LIB "/app/lib/libboost_system.so")
            else()
                find_library(BOOST_SYSTEM_LIB
                    NAMES boost_system
                    PATHS /app/lib /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
                    NO_DEFAULT_PATH
                )
                if(NOT BOOST_SYSTEM_LIB)
                    find_library(BOOST_SYSTEM_LIB NAMES boost_system)
                endif()
            endif()
            
            if(EXISTS "/app/lib/libboost_filesystem.so")
                set(BOOST_FILESYSTEM_LIB "/app/lib/libboost_filesystem.so")
            else()
                find_library(BOOST_FILESYSTEM_LIB
                    NAMES boost_filesystem
                    PATHS /app/lib /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
                    NO_DEFAULT_PATH
                )
                if(NOT BOOST_FILESYSTEM_LIB)
                    find_library(BOOST_FILESYSTEM_LIB NAMES boost_filesystem)
                endif()
            endif()
            
            # Link libtorrent and its boost dependencies
            target_link_libraries(Yantrium PRIVATE ${LIBTORRENT_LIB})
            if(BOOST_SYSTEM_LIB)
                message(STATUS "Linking boost_system: ${BOOST_SYSTEM_LIB}")
                target_link_libraries(Yantrium PRIVATE ${BOOST_SYSTEM_LIB})
            else()
                message(WARNING "boost_system not found - libtorrent may not link correctly")
            endif()
            if(BOOST_FILESYSTEM_LIB)
                message(STATUS "Linking boost_filesystem: ${BOOST_FILESYSTEM_LIB}")
                target_link_libraries(Yantrium PRIVATE ${BOOST_FILESYSTEM_LIB})
            else()
                message(WARNING "boost_filesystem not found - libtorrent may not link correctly")
            endif()
        else()
            message(WARNING "libtorrent not found - torrent support disabled")
            message(STATUS "Searched for library in: /app/lib, /usr/lib, /usr/local/lib")
            message(STATUS "Searched for headers in: /app/include, /usr/include, /usr/local/include")
            if(LIBTORRENT_LIB)
                message(STATUS "Found library but no headers: ${LIBTORRENT_LIB}")
            endif()
            if(LIBTORRENT_INCLUDE_DIR)
                message(STATUS "Found headers but no library: ${LIBTORRENT_INCLUDE_DIR}")
            endif()
        endif()
    endif()
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