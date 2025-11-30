cmake_minimum_required(VERSION 3.16)

project(Yantrium VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. ENABLE QT AUTOMATION (Fixes vtable errors)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 2. Find Qt6 Packages
find_package(Qt6 REQUIRED COMPONENTS Quick Widgets Network Gui Sql)

# 3. Add Source Files
set(PROJECT_SOURCES
    src/main.cpp
    src/app_controller.cpp
    src/app_controller.h
    src/player/player_bridge.cpp
    src/player/player_bridge.h
    src/player/mdk_player.cpp
    src/player/mdk_player.h
    # Core database
    src/core/database/database_manager.cpp
    src/core/database/database_manager.h
    src/core/database/addon_dao.cpp
    src/core/database/addon_dao.h
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
    src/core/models/tmdb_models.cpp
    src/core/models/tmdb_models.h
    # Core services
    src/core/services/configuration.cpp
    src/core/services/configuration.h
    src/core/services/id_parser.cpp
    src/core/services/id_parser.h
    src/core/services/tmdb_data_extractor.cpp
    src/core/services/tmdb_data_extractor.h
    src/core/services/tmdb_service.cpp
    src/core/services/tmdb_service.h
    src/core/services/tmdb_search_service.cpp
    src/core/services/tmdb_search_service.h
    resources/qml.qrc
)

# 4. Define the Executable
qt_add_executable(Yantrium
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

# Add src directory to include path for cleaner includes
target_include_directories(Yantrium PRIVATE src)

# TMDB API Key configuration
option(TMDB_API_KEY "TMDB API Key" "")
if(TMDB_API_KEY)
    target_compile_definitions(Yantrium PRIVATE TMDB_API_KEY="${TMDB_API_KEY}")
    message(STATUS "TMDB API Key configured")
else()
    message(WARNING "TMDB_API_KEY not set. Set it via: -DTMDB_API_KEY=your_key")
    target_compile_definitions(Yantrium PRIVATE TMDB_API_KEY="")
endif()

# 5. Link Qt Libraries
target_link_libraries(Yantrium PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::Widgets
    Qt6::Network
    Qt6::Gui
    Qt6::Sql
)

# =========================================================
# 6. MDK-SDK INTEGRATION
# =========================================================

# Define where MDK is located
set(MDK_DIR "${CMAKE_SOURCE_DIR}/vendor/mdk-sdk")

if(WIN32)
    target_include_directories(Yantrium PRIVATE ${MDK_DIR}/include)
    target_link_libraries(Yantrium PRIVATE ${MDK_DIR}/lib/x64/mdk.lib)
    
    add_custom_command(TARGET Yantrium POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${MDK_DIR}/bin/x64/ $<TARGET_FILE_DIR:Yantrium>)
        
elseif(UNIX AND NOT APPLE)
    target_include_directories(Yantrium PRIVATE ${MDK_DIR}/include)
    
    # Fixed path based on your earlier 'find' command
    target_link_libraries(Yantrium PRIVATE ${MDK_DIR}/lib/amd64/libmdk.so)
    
    add_custom_command(TARGET Yantrium POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${MDK_DIR}/lib/amd64/libmdk.so $<TARGET_FILE_DIR:Yantrium>)
        
    set_target_properties(Yantrium PROPERTIES
        BUILD_RPATH "\$ORIGIN"
        INSTALL_RPATH "\$ORIGIN"
    )
endif()

qt_import_qml_plugins(Yantrium)