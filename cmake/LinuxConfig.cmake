# Linux-specific configuration

# =========================================================
# MDK-SDK INTEGRATION (Linux)
# =========================================================
set(MDK_DIR "${CMAKE_SOURCE_DIR}/vendor/mdk-sdk")

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

# =========================================================
# LIBTORRENT INTEGRATION (Linux)
# =========================================================
set(LIBTORRENT_DIR "${CMAKE_SOURCE_DIR}/vendor/libtorrent")

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
    # ==================================================
    # SYSTEM LIBTORRENT FALLBACK (FLATPAK/LINUX)
    # ==================================================
    
    # 1. Try PkgConfig with IMPORTED_TARGET option (Most reliable)
    if(PkgConfig_FOUND)
        pkg_check_modules(LIBTORRENT IMPORTED_TARGET libtorrent-rasterbar)
    endif()

    if(LIBTORRENT_FOUND)
        message(STATUS "--- Libtorrent Found via PkgConfig ---")
        message(STATUS "Linking: PkgConfig::LIBTORRENT")
        
        # Link using the PkgConfig imported target
        target_link_libraries(Yantrium PRIVATE PkgConfig::LIBTORRENT)
        
        # PkgConfig usually sets includes, but for completeness:
        target_include_directories(Yantrium PRIVATE ${LIBTORRENT_INCLUDE_DIRS})
        target_compile_definitions(Yantrium PRIVATE TORRENT_SUPPORT_ENABLED)
        
    else()
        # 2. Manual Fallback: Find the absolute library path
        message(STATUS "PkgConfig failed, trying manual absolute path link...")
        
        find_library(LIBTORRENT_LIB_FILE 
            NAMES libtorrent-rasterbar.so libtorrent-rasterbar.a torrent-rasterbar
            PATHS /app/lib /usr/lib /usr/local/lib /usr/lib64
        )
        
        find_path(LIBTORRENT_INCLUDE_DIR
            NAMES libtorrent/session.hpp
            PATHS /app/include /usr/include /usr/local/include
        )

        if(LIBTORRENT_LIB_FILE AND LIBTORRENT_INCLUDE_DIR)
            message(STATUS "Found libtorrent manually: ${LIBTORRENT_LIB_FILE}")
            target_include_directories(Yantrium PRIVATE ${LIBTORRENT_INCLUDE_DIR})
            
            # Link using the FULL PATH to the .so file
            target_link_libraries(Yantrium PRIVATE ${LIBTORRENT_LIB_FILE})
            
            target_compile_definitions(Yantrium PRIVATE TORRENT_SUPPORT_ENABLED)
        else()
            message(STATUS "libtorrent not found - torrent support disabled")
        endif()
    endif()
endif()
