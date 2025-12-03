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
            
            // Navigation bar
            Loader {
                id: navBarLoader
                width: parent.width
                height: 60
                source: "qrc:/qml/components/NavigationBar.qml"
                
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

                        // Try to trigger search immediately if loader is ready
                        if (searchLoader.item && query) {
                            console.warn("[MainApp] Search loader ready, triggering search immediately")
                            searchLoader.item.searchQuery = query
                            searchLoader.item.performSearch()
                        } else {
                            console.warn("[MainApp] Search loader not ready yet, will trigger when loaded")
                            console.warn("[MainApp] searchLoader.item:", searchLoader.item)
                            console.warn("[MainApp] searchLoader.status:", searchLoader.status)
                        }
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
                height: parent.height - navBarLoader.height
                currentIndex: 0

                onCurrentIndexChanged: {
                    console.warn("[MainApp] ===== STACK LAYOUT INDEX CHANGED =====")
                    console.warn("[MainApp] New index:", currentIndex)

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

                    // Reload when switching to home tab
                    Connections {
                        target: stackLayout
                        function onCurrentIndexChanged() {
                            if (stackLayout.currentIndex === 0 && homeLoader.item) {
                                homeLoader.item.loadCatalogs()
                            } else if (stackLayout.currentIndex === 1 && libraryLoader.item) {
                                // Reload library when switching to library tab
                                libraryLoader.item.loadLibrary()
                            }
                        }
                    }
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
                        if (status === Loader.Ready && item && pendingQuery && pendingQuery.length > 0) {
                            console.warn("[MainApp] Search loader ready, triggering search with pending query:", pendingQuery)
                            item.searchQuery = pendingQuery
                            item.performSearch()
                            pendingQuery = ""
                        }
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

                            // Load details if we have pending data
                            if (pendingContentId && pendingType) {
                                // Switch to detail screen first
                                stackLayout.currentIndex = 4
                                // Then load details
                                item.loadDetails(pendingContentId, pendingType, pendingAddonId)
                                pendingContentId = ""
                                pendingType = ""
                                pendingAddonId = ""
                            }
                        }
                    }
                    
                    onStatusChanged: {
                        // When detail loader becomes ready and we have pending data, switch to it
                        if (status === Loader.Ready && item && pendingContentId && pendingType) {
                            stackLayout.currentIndex = 4
                            item.loadDetails(pendingContentId, pendingType, pendingAddonId)
                            pendingContentId = ""
                            pendingType = ""
                            pendingAddonId = ""
                        }
                    }
                }
                
                Connections {
                    target: detailLoader.item
                    function onPlayRequested(streamUrl) {
                        console.log("[MainApp] Play requested with URL:", streamUrl)
                        // Activate video player loader
                        videoPlayerLoader.pendingStreamUrl = streamUrl
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
                    
                    onLoaded: {
                        if (item && pendingStreamUrl) {
                            console.log("[MainApp] Video player loaded, setting source:", pendingStreamUrl)
                            item.videoPlayer.source = pendingStreamUrl
                            // Auto-play
                            Qt.callLater(function() {
                                item.videoPlayer.play()
                            })
                            pendingStreamUrl = ""
                        }
                    }
                    
                    onStatusChanged: {
                        if (status === Loader.Ready && item && pendingStreamUrl) {
                            console.log("[MainApp] Video player ready, setting source:", pendingStreamUrl)
                            item.videoPlayer.source = pendingStreamUrl
                            // Auto-play
                            Qt.callLater(function() {
                                item.videoPlayer.play()
                            })
                            pendingStreamUrl = ""
                        }
                    }
                }
            }
        }
    }
}

