# Windows-specific configuration

# Add Windows resource file
list(APPEND PROJECT_SOURCES resources/yantrium.rc)

# =========================================================
# MDK-SDK INTEGRATION (Windows)
# =========================================================
set(MDK_DIR "${CMAKE_SOURCE_DIR}/vendor/mdk-sdk")

target_include_directories(Yantrium PRIVATE ${MDK_DIR}/include)
target_link_libraries(Yantrium PRIVATE ${MDK_DIR}/lib/x64/mdk.lib)

# Copy MDK binaries to build directory on Windows
add_custom_command(TARGET Yantrium POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${MDK_DIR}/bin/x64/ $<TARGET_FILE_DIR:Yantrium>)

# =========================================================
# LIBTORRENT INTEGRATION (Windows)
# =========================================================
set(LIBTORRENT_DIR "${CMAKE_SOURCE_DIR}/vendor/libtorrent")

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
