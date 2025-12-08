import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Yantrium.Services 1.0

ApplicationWindow {
    id: root
    width: 1280
    height: 720
    visible: true
    title: "Yantrium"
    
    // Trakt services
    property TraktAuthService traktAuthService: TraktAuthService
    property TraktCoreService traktService: TraktCoreService
    
    // Sync Trakt watch history on authentication
    Connections {
        target: traktAuthService
        function onAuthenticationStatusChanged(authenticated) {
            if (authenticated) {
                console.log("[MainApp] Trakt authenticated, starting watch history sync")
                // Sync watched movies and shows
                traktService.syncWatchedMovies()
                traktService.syncWatchedShows()
            }
        }
    }
    
    // Trigger sync on app startup if already authenticated
    Component.onCompleted: {
        if (traktAuthService.isAuthenticated) {
            console.log("[MainApp] Already authenticated on startup, starting watch history sync")
            traktService.syncWatchedMovies()
            traktService.syncWatchedShows()
        }
    }
    
    Rectangle {
        anchors.fill: parent
        color: "#09090b"
        
        Column {
            anchors.fill: parent
            spacing: 0
            
            // Navigation bar - hidden when video player is active (index 5)
            Loader {
                id: navBarLoader
                width: parent.width
                height: 60
                source: "qrc:/qml/components/NavigationBar.qml"
                visible: stackLayout.currentIndex !== 5
                
                Connections {
                    target: navBarLoader.item
                    function onTabClicked(index) {
                        console.warn("[MainApp] ===== TAB CLICKED =====")
                        console.warn("[MainApp] Tab index:", index)
                        console.warn("[MainApp] Current stackLayout index before:", stackLayout.currentIndex)
                        stackLayout.currentIndex = index
                        console.warn("[MainApp] Current stackLayout index after:", stackLayout.currentIndex)
                    }
                    function onSearchClicked(query) {
                        console.warn("[MainApp] ===== SEARCH CLICKED =====")
                        console.warn("[MainApp] Search query:", query)
                        console.warn("[MainApp] Current stackLayout index before:", stackLayout.currentIndex)

                        // Store the query for when the screen loads
                        searchLoader.pendingQuery = query

                        // Activate search loader if not active
                        if (!searchLoader.active) {
                            console.warn("[MainApp] Activating search loader")
                            searchLoader.active = true
                        }

                        stackLayout.currentIndex = 3  // Search screen is at index 3
                        console.warn("[MainApp] Current stackLayout index after:", stackLayout.currentIndex)
                    }
                }
                
                onLoaded: {
                    console.warn("[MainApp] ===== NAV BAR LOADED =====")
                    console.warn("[MainApp] Nav bar item:", item)
                    if (item) {
                        console.warn("[MainApp] Setting up nav bar currentIndex binding")
                        item.currentIndex = Qt.binding(function() { return stackLayout.currentIndex })
                    }
                }
            }
            
            // Main content area
            StackLayout {
                id: stackLayout
                width: parent.width
                height: parent.height - (navBarLoader.visible ? navBarLoader.height : 0)
                currentIndex: 0

                onCurrentIndexChanged: {
                    console.warn("[MainApp] ===== STACK LAYOUT INDEX CHANGED =====")
                    console.warn("[MainApp] New index:", currentIndex)
                    console.warn("[MainApp] Previous index:", videoPlayerLoader.previousIndex)
                    
                    // Stop video playback if leaving the video player (index 5)
                    // Only stop if we were previously on the video player and are now leaving it
                    if (videoPlayerLoader.previousIndex === 5 && currentIndex !== 5) {
                        if (videoPlayerLoader.item && videoPlayerLoader.item.videoPlayer) {
                            console.log("[MainApp] Leaving video player, stopping playback")
                            videoPlayerLoader.item.videoPlayer.stop()
                        }
                        
                        // Refresh home page if returning to it after watching content
                        if (currentIndex === 0 && homeLoader.item) {
                            console.log("[MainApp] Returning to home after watching, refreshing to update Continue Watching")
                            homeLoader.item.loadCatalogs()
                        }
                    }
                    
                    // Update previous index
                    videoPlayerLoader.previousIndex = currentIndex

                    // Activate search loader when switching to search screen
                    if (currentIndex === 3 && !searchLoader.active) {
                        console.warn("[MainApp] Activating search loader for index 3")
                        searchLoader.active = true
                    }
                }

                Loader {
                    id: homeLoader
                    source: "qrc:/qml/screens/HomeScreen.qml"

                    onLoaded: {
                        if (item) {
                            item.loadCatalogs()
                            // Connect itemClicked signal to show detail screen
                            item.itemClicked.connect(function(contentId, type, addonId) {
                                console.log("[MainApp] HomeScreen itemClicked - contentId:", contentId, "type:", type, "addonId:", addonId)
                                if (contentId && type) {
                                    // Always activate loader first
                                    detailLoader.active = true
                                    
                                    // Store pending data
                                    detailLoader.pendingContentId = contentId
                                    detailLoader.pendingType = type
                                    detailLoader.pendingAddonId = addonId || ""
                                    
                                    // If already loaded, switch immediately and load
                                    if (detailLoader.status === Loader.Ready && detailLoader.item) {
                                        console.log("[MainApp] DetailScreen already loaded, calling loadDetails with contentId:", contentId)
                                        stackLayout.currentIndex = 4
                                        detailLoader.item.loadDetails(contentId, type, addonId || "")
                                        detailLoader.pendingContentId = ""
                                        detailLoader.pendingType = ""
                                        detailLoader.pendingAddonId = ""
                                    } else {
                                        console.log("[MainApp] DetailScreen not ready yet, will load when ready. Status:", detailLoader.status)
                                    }
                                } else {
                                    console.error("[MainApp] Invalid itemClicked - missing contentId or type. contentId:", contentId, "type:", type)
                                }
                            })
                            // Connect playRequested signal to video player
                            item.playRequested.connect(function(streamUrl) {
                                console.log("[MainApp] HomeScreen play requested with URL:", streamUrl)
                                // Activate video player loader
                                videoPlayerLoader.pendingStreamUrl = streamUrl
                                videoPlayerLoader.active = true
                                // Switch to video player (index 5)
                                stackLayout.currentIndex = 5
                            })
                        }
                    }

                    // Reload library when switching to library tab
                    Connections {
                        target: stackLayout
                        function onCurrentIndexChanged() {
                            if (stackLayout.currentIndex === 1 && libraryLoader.item) {
                                // Reload library when switching to library tab
                                libraryLoader.item.loadLibrary()
                            }
                        }
                    }
                    
                    // Track previous index to detect when returning from video player
                    property int previousIndex: 0
                }
                
                Loader {
                    id: libraryLoader
                    source: "qrc:/qml/screens/LibraryScreen.qml"
                    
                    onLoaded: {
                        if (item) {
                            // Connect itemClicked signal to show detail screen
                            item.itemClicked.connect(function(contentId, type, addonId) {
                                if (contentId && type) {
                                    // Store pending data
                                    detailLoader.pendingContentId = contentId
                                    detailLoader.pendingType = type
                                    detailLoader.pendingAddonId = addonId || ""
                                    
                                    // Store pending data and activate loader
                                    // The switch will happen in onStatusChanged when loader is ready
                                    detailLoader.pendingContentId = contentId
                                    detailLoader.pendingType = type
                                    detailLoader.pendingAddonId = addonId || ""
                                    detailLoader.active = true
                                    
                                    // If already loaded, switch immediately
                                    if (detailLoader.status === Loader.Ready && detailLoader.item) {
                                        stackLayout.currentIndex = 4
                                        detailLoader.item.loadDetails(contentId, type, addonId || "")
                                        detailLoader.pendingContentId = ""
                                        detailLoader.pendingType = ""
                                        detailLoader.pendingAddonId = ""
                                    }
                                }
                            })
                        }
                    }
                }
                
                Loader {
                    source: "qrc:/qml/screens/SettingsScreen.qml"
                }
                
                // Search Screen
                Loader {
                    id: searchLoader
                    source: "qrc:/qml/screens/SearchScreen.qml"
                    active: false  // Start inactive, will be activated when needed
                    property string pendingQuery: ""  // Store query until screen loads

                    onLoaded: {
                        console.warn("[MainApp] ===== SEARCH LOADER LOADED =====")
                        console.warn("[MainApp] Search screen loaded successfully")
                        if (item) {
                            console.warn("[MainApp] Search screen item exists")
                            
                            // If we have a pending query, trigger search now
                            if (pendingQuery && pendingQuery.length > 0) {
                                console.warn("[MainApp] Triggering search with pending query:", pendingQuery)
                                item.searchQuery = pendingQuery
                                item.performSearch()
                                pendingQuery = ""  // Clear after using
                            }
                            
                            // Connect itemClicked signal to show detail screen
                            item.itemClicked.connect(function(contentId, type, addonId) {
                                console.warn("[MainApp] Search item clicked:", contentId, type, addonId)
                                if (contentId && type) {
                                    // Store pending data
                                    detailLoader.pendingContentId = contentId
                                    detailLoader.pendingType = type
                                    detailLoader.pendingAddonId = addonId || ""

                                    // Store pending data and activate loader
                                    // The switch will happen in onStatusChanged when loader is ready
                                    detailLoader.pendingContentId = contentId
                                    detailLoader.pendingType = type
                                    detailLoader.pendingAddonId = addonId || ""
                                    detailLoader.active = true
                                    
                                    // If already loaded, switch immediately
                                    if (detailLoader.status === Loader.Ready && detailLoader.item) {
                                        stackLayout.currentIndex = 4
                                        detailLoader.item.loadDetails(contentId, type, addonId || "")
                                        detailLoader.pendingContentId = ""
                                        detailLoader.pendingType = ""
                                        detailLoader.pendingAddonId = ""
                                    }
                                }
                            })
                        } else {
                            console.warn("[MainApp] ERROR: Search screen item is null")
                        }
                    }

                    onActiveChanged: {
                        console.warn("[MainApp] Search loader active changed:", active)
                    }

                    onStatusChanged: {
                        console.warn("[MainApp] Search loader status changed:", status)
                    }
                }
                
                // Detail Screen
                Loader {
                    id: detailLoader
                    active: false
                    source: "qrc:/qml/screens/DetailScreen.qml"
                    
                    property string pendingContentId: ""
                    property string pendingType: ""
                    property string pendingAddonId: ""
                    
                    onLoaded: {
                        if (item) {
                            // Connect closeRequested to go back to previous screen
                            item.closeRequested.connect(function() {
                                // Go back to the screen we came from (home, library, or search)
                                if (stackLayout.currentIndex === 4) {
                                    stackLayout.currentIndex = 0  // Default to home
                                }
                                detailLoader.active = false
                            })

                            // Connect libraryChanged to refresh library with a delay
                            item.libraryChanged.connect(function() {
                                console.log("[MainApp] Library change detected, scheduling refresh in 500ms")
                                // Add a small delay to ensure database/API operations complete
                                var refreshTimer = Qt.createQmlObject("import QtQuick; Timer { interval: 500; running: true; repeat: false }", root)
                                refreshTimer.triggered.connect(function() {
                                    if (libraryLoader.item && typeof libraryLoader.item.loadLibrary === 'function') {
                                        console.log("[MainApp] Refreshing library after library change")
                                        // Clear Trakt cache to force fresh API calls
                                        TraktCoreService.clearCache()
                                        libraryLoader.item.loadLibrary()
                                    }
                                    refreshTimer.destroy()
                                })
                            })

                            // Connect showRequested to navigate back to show from episode
                            item.showRequested.connect(function(contentId, type, addonId) {
                                console.log("[MainApp] Show requested from episode - contentId:", contentId, "type:", type)
                                if (contentId && type) {
                                    // Load show details
                                    item.loadDetails(contentId, type, addonId || "")
                                }
                            })

                            // Load details if we have pending data
                            if (detailLoader.pendingContentId && detailLoader.pendingType) {
                                console.log("[MainApp] DetailScreen loaded with pending data - contentId:", detailLoader.pendingContentId)
                                // Switch to detail screen first
                                stackLayout.currentIndex = 4
                                // Then load details
                                item.loadDetails(detailLoader.pendingContentId, detailLoader.pendingType, detailLoader.pendingAddonId)
                                detailLoader.pendingContentId = ""
                                detailLoader.pendingType = ""
                                detailLoader.pendingAddonId = ""
                            }
                        }
                    }
                    
                    onStatusChanged: {
                        if (status === Loader.Ready && item && detailLoader.pendingContentId && detailLoader.pendingType) {
                            console.log("[MainApp] DetailScreen became ready with pending data - contentId:", detailLoader.pendingContentId)
                            stackLayout.currentIndex = 4
                            item.loadDetails(detailLoader.pendingContentId, detailLoader.pendingType, detailLoader.pendingAddonId)
                            detailLoader.pendingContentId = ""
                            detailLoader.pendingType = ""
                            detailLoader.pendingAddonId = ""
                        }
                    }
                }
                
                Connections {
                    target: detailLoader.item
                    function onPlayRequested(streamUrl, contentData) {
                        console.log("[MainApp] Play requested with URL:", streamUrl)
                        console.log("[MainApp] Content data:", JSON.stringify(contentData))
                        // Activate video player loader
                        videoPlayerLoader.pendingStreamUrl = streamUrl
                        videoPlayerLoader.pendingContentData = contentData || {}
                        videoPlayerLoader.active = true
                        // Switch to video player (index 5)
                        stackLayout.currentIndex = 5
                    }
                }
                
                // Video Player Screen
                Loader {
                    id: videoPlayerLoader
                    active: false
                    source: "qrc:/qml/screens/VideoPlayerScreen.qml"
                    
                    property string pendingStreamUrl: ""
                    property var pendingContentData: ({})
                    property int previousIndex: -1
                    
                    onLoaded: {
                        if (item && pendingStreamUrl) {
                            console.log("[MainApp] Video player loaded, setting source:", pendingStreamUrl)
                            // Set content data
                            if (pendingContentData) {
                                item.contentType = pendingContentData.type || ""
                                item.contentTitle = pendingContentData.title || ""
                                item.contentDescription = pendingContentData.description || ""
                                item.logoUrl = pendingContentData.logoUrl || ""
                                item.seasonNumber = pendingContentData.seasonNumber || 0
                                item.episodeNumber = pendingContentData.episodeNumber || 0
                            }
                            item.videoPlayer.source = pendingStreamUrl
                            // Auto-play
                            Qt.callLater(function() {
                                item.videoPlayer.play()
                            })
                            pendingStreamUrl = ""
                            pendingContentData = {}
                        }
                    }
                    
                    onStatusChanged: {
                        if (status === Loader.Ready && item && pendingStreamUrl) {
                            console.log("[MainApp] Video player ready, setting source:", pendingStreamUrl)
                            // Set content data
                            if (pendingContentData) {
                                item.contentType = pendingContentData.type || ""
                                item.contentTitle = pendingContentData.title || ""
                                item.contentDescription = pendingContentData.description || ""
                                item.logoUrl = pendingContentData.logoUrl || ""
                                item.seasonNumber = pendingContentData.seasonNumber || 0
                                item.episodeNumber = pendingContentData.episodeNumber || 0
                            }
                            item.videoPlayer.source = pendingStreamUrl
                            // Auto-play
                            Qt.callLater(function() {
                                item.videoPlayer.play()
                            })
                            pendingStreamUrl = ""
                            pendingContentData = {}
                        }
                    }
                    
                    Connections {
                        target: videoPlayerLoader.item
                        function onGoBackRequested() {
                            console.log("[MainApp] Go back requested from video player")
                            // Stop video playback before leaving
                            if (videoPlayerLoader.item && videoPlayerLoader.item.videoPlayer) {
                                console.log("[MainApp] Stopping video playback")
                                videoPlayerLoader.item.videoPlayer.stop()
                            }
                            // Refresh home page to update "Continue Watching" after watching content
                            if (homeLoader.item) {
                                console.log("[MainApp] Refreshing home page after watching content")
                                homeLoader.item.loadCatalogs()
                            }
                            stackLayout.currentIndex = 4  // Go back to detail screen
                        }
                        
                        function onToggleFullscreenRequested() {
                            console.log("[MainApp] Toggle fullscreen requested")
                            if (root.visibility === ApplicationWindow.FullScreen) {
                                root.showNormal()
                            } else {
                                root.showFullScreen()
                            }
                        }
                    }
                }
            }
        }
    }
}

