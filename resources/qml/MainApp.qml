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
    
    // Navigation service (includes screen management)
    property NavigationService navigationService: NavigationService
    
    // Infrastructure services
    property LoggingService loggingService: LoggingService
    
    // Connect LoggingService to handle errors globally
    Connections {
        target: loggingService
        function onErrorOccurred(message, code, context) {
            // Error logged by LoggingService internally
            // TODO: Display error to user (toast, dialog, etc.)
        }
    }
    
    // Sync Trakt watch history on authentication
    Connections {
        target: traktAuthService
        function onAuthenticationStatusChanged(authenticated) {
            if (authenticated) {
                loggingService.info("MainApp", "Trakt authenticated, starting watch history sync")
                // Sync watched movies and shows
                traktService.syncWatchedMovies()
                traktService.syncWatchedShows()
            }
        }
    }
    
    // Trigger sync on app startup if already authenticated
    Component.onCompleted: {
        if (traktAuthService.isAuthenticated) {
            loggingService.info("MainApp", "Already authenticated on startup, starting watch history sync")
            traktService.syncWatchedMovies()
            traktService.syncWatchedShows()
        }
    }
    
    // Connect NavigationService signals
    Connections {
        target: navigationService
        function onDetailRequested(contentId, type, addonId) {
            if (contentId && type) {
                // Store pending data
                detailLoader.pendingContentId = contentId
                detailLoader.pendingType = type
                detailLoader.pendingAddonId = addonId || ""
                detailLoader.pendingSeason = 0
                detailLoader.pendingEpisode = 0
                detailLoader.active = true
                
                if (detailLoader.status === Loader.Ready && detailLoader.item) {
                    detailLoader.item.loadDetails(contentId, type, addonId || "", 0, 0)
                    detailLoader.pendingContentId = ""
                    detailLoader.pendingType = ""
                    detailLoader.pendingAddonId = ""
                    navigationService.navigateTo(NavigationService.Detail)
                } else {
                    // Navigation will happen when loader is ready (handled in onStatusChanged)
                    navigationService.navigateTo(NavigationService.Detail)
                }
            }
        }
        function onDetailRequested(contentId, type, addonId, season, episode) {
            if (contentId && type) {
                // Store pending data
                detailLoader.pendingContentId = contentId
                detailLoader.pendingType = type
                detailLoader.pendingAddonId = addonId || ""
                detailLoader.pendingSeason = season || 0
                detailLoader.pendingEpisode = episode || 0
                detailLoader.active = true
                
                if (detailLoader.status === Loader.Ready && detailLoader.item) {
                    detailLoader.item.loadDetails(contentId, type, addonId || "", season || 0, episode || 0)
                    detailLoader.pendingContentId = ""
                    detailLoader.pendingType = ""
                    detailLoader.pendingAddonId = ""
                    detailLoader.pendingSeason = 0
                    detailLoader.pendingEpisode = 0
                    navigationService.navigateTo(NavigationService.Detail)
                } else {
                    // Navigation will happen when loader is ready (handled in onStatusChanged)
                    navigationService.navigateTo(NavigationService.Detail)
                }
            }
        }
        function onPlayerRequested(streamUrl, contentData) {
            // Removed debug log - unnecessary
            // Store pending data
            videoPlayerLoader.pendingStreamUrl = streamUrl
            videoPlayerLoader.pendingContentData = contentData || ({})
            videoPlayerLoader.active = true
            
            if (videoPlayerLoader.status === Loader.Ready && videoPlayerLoader.item) {
                if (videoPlayerLoader.pendingContentData) {
                    videoPlayerLoader.item.contentType = videoPlayerLoader.pendingContentData.type || ""
                    videoPlayerLoader.item.contentTitle = videoPlayerLoader.pendingContentData.title || ""
                    videoPlayerLoader.item.contentDescription = videoPlayerLoader.pendingContentData.description || ""
                    videoPlayerLoader.item.logoUrl = videoPlayerLoader.pendingContentData.logoUrl || ""
                    videoPlayerLoader.item.seasonNumber = videoPlayerLoader.pendingContentData.seasonNumber || 0
                    videoPlayerLoader.item.episodeNumber = videoPlayerLoader.pendingContentData.episodeNumber || 0
                }
                videoPlayerLoader.item.videoPlayer.source = streamUrl
                Qt.callLater(function() {
                    videoPlayerLoader.item.videoPlayer.play()
                })
                videoPlayerLoader.pendingStreamUrl = ""
                videoPlayerLoader.pendingContentData = {}
                navigationService.navigateTo(NavigationService.Player)
            } else {
                // Navigation will happen when loader is ready (handled in onStatusChanged)
                navigationService.navigateTo(NavigationService.Player)
            }
        }
        function onSearchRequested(query) {
            loggingService.debug("MainApp", "NavigationService: Search requested - query: " + query)
            searchLoader.active = true
            navigationService.navigateTo(NavigationService.Search)
            // Use a timer to set query when loader is ready
            if (query) {
                var queryTimer = Qt.createQmlObject("import QtQuick; Timer { interval: 100; running: true; repeat: true }", root)
                queryTimer.triggered.connect(function() {
                    if (searchLoader.status === Loader.Ready && searchLoader.item) {
                        searchLoader.item.searchQuery = query
                        searchLoader.item.performSearch()
                        queryTimer.stop()
                        queryTimer.destroy()
                    }
                })
            }
        }
        function onBackRequested() {
            // Handle detail screen cleanup when going back
            if (navigationService.currentScreen === NavigationService.Detail) {
                detailLoader.active = false
            }
            // Don't call navigateBack() here - it's already been handled by NavigationService
            // Calling it again would create an infinite loop
        }
    }
    
    // Connect NavigationService screen change signals
    Connections {
        target: navigationService
        function onScreenChangeRequested(screenIndex) {
            // Handle any screen-specific logic here if needed
            loggingService.debug("MainApp", "Screen change requested to: " + screenIndex)
        }
    }
    
    Rectangle {
        anchors.fill: parent
        color: "#09090b"
        
        Column {
            anchors.fill: parent
            spacing: 0
            
            // Navigation bar - hidden when video player is active
            Loader {
                id: navBarLoader
                width: parent.width
                height: 60
                source: "qrc:/qml/components/NavigationBar.qml"
                visible: navigationService.currentScreen !== NavigationService.Player
                
                function updateBackButtonVisibility() {
                    if (navBarLoader.item) {
                        // Show back button for Search (3) and Detail (4) screens
                        // navigateBack() will handle the case where we can't go back
                        var isSearchOrDetail = navigationService.currentScreen === NavigationService.Search || navigationService.currentScreen === NavigationService.Detail
                        navBarLoader.item.showBackButton = isSearchOrDetail
                    }
                }
                
                Connections {
                    target: navBarLoader.item
                    function onTabClicked(index) {
                        navigationService.navigateToIndex(index)
                    }
                    function onSearchClicked(query) {
                        // Removed debug logs - unnecessary
                        navigationService.navigateToSearch(query)
                    }
                    function onBackClicked() {
                        navigationService.navigateBack()
                    }
                }
                
                Connections {
                    target: navigationService
                    function onCurrentScreenChanged() {
                        navBarLoader.updateBackButtonVisibility()
                    }
                }
                
                onLoaded: {
                    if (item) {
                        item.currentIndex = Qt.binding(function() { return navigationService.currentScreen })
                        updateBackButtonVisibility()
                    }
                }
            }
            
            // Main content area
            StackLayout {
                id: stackLayout
                width: parent.width
                height: parent.height - (navBarLoader.visible ? navBarLoader.height : 0)
                currentIndex: navigationService.currentScreen

                property int previousScreen: NavigationService.Home
                
                onCurrentIndexChanged: {
                    // Stop video playback if leaving the video player
                    if (previousScreen === NavigationService.Player && currentIndex !== NavigationService.Player) {
                        if (videoPlayerLoader.item && videoPlayerLoader.item.videoPlayer) {
                            videoPlayerLoader.item.videoPlayer.stop()
                        }
                        
                        // Refresh home page if returning to it after watching content
                        if (currentIndex === NavigationService.Home && homeLoader.item) {
                            homeLoader.item.loadCatalogs()
                        }
                    }
                    
                    // Update previous screen
                    previousScreen = currentIndex

                    // Activate search loader when switching to search screen
                    if (currentIndex === NavigationService.Search && !searchLoader.active) {
                        searchLoader.active = true
                    }
                }

                Loader {
                    id: homeLoader
                    source: "qrc:/qml/screens/HomeScreen.qml"

                    onLoaded: {
                        if (item) {
                            item.loadCatalogs()
                            // Connect itemClicked signal to NavigationService
                            item.itemClicked.connect(function(contentId, type, addonId) {
                                // Removed debug log - unnecessary
                                navigationService.navigateToDetail(contentId, type, addonId || "")
                            })
                            // Connect playRequested signal to NavigationService
                            item.playRequested.connect(function(streamUrl, contentData) {
                                navigationService.navigateToPlayer(streamUrl, contentData || ({}))
                            })
                        }
                    }

                    // Reload library when switching to library tab
                    Connections {
                        target: navigationService
                        function onCurrentScreenChanged(screen) {
                            if (screen === NavigationService.Library && libraryLoader.item) {
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
                            // Connect itemClicked signal to NavigationService
                            item.itemClicked.connect(function(contentId, type, addonId) {
                                navigationService.navigateToDetail(contentId, type, addonId || "")
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

                    onLoaded: {
                        if (item) {
                            // Removed debug logs - unnecessary
                            
                            // Connect closeRequested to NavigationService
                            item.closeRequested.connect(function() {
                                navigationService.navigateBack()
                            })

                            // Connect itemClicked signal to NavigationService
                            item.itemClicked.connect(function(contentId, type, addonId) {
                                navigationService.navigateToDetail(contentId, type, addonId || "")
                            })
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
                    property int pendingSeason: 0
                    property int pendingEpisode: 0
                    
                    onLoaded: {
                        if (item) {
                            // Connect closeRequested to NavigationService
                            item.closeRequested.connect(function() {
                                navigationService.navigateBack()
                                detailLoader.active = false
                            })

                            // Connect libraryChanged to refresh library with a delay
                            item.libraryChanged.connect(function() {
                                // Add a small delay to ensure database/API operations complete
                                var refreshTimer = Qt.createQmlObject("import QtQuick; Timer { interval: 500; running: true; repeat: false }", root)
                                refreshTimer.triggered.connect(function() {
                                    if (libraryLoader.item && typeof libraryLoader.item.loadLibrary === 'function') {
                                        // Clear Trakt cache to force fresh API calls
                                        TraktCoreService.clearCache()
                                        libraryLoader.item.loadLibrary()
                                    }
                                    refreshTimer.destroy()
                                })
                            })

                            // Connect showRequested to navigate back to show from episode
                            item.showRequested.connect(function(contentId, type, addonId) {
                                if (contentId && type) {
                                    // Load show details
                                    item.loadDetails(contentId, type, addonId || "")
                                }
                            })

                            // Load details if we have pending data (from NavigationService)
                            if (pendingContentId && pendingType) {
                                item.loadDetails(pendingContentId, pendingType, pendingAddonId, pendingSeason, pendingEpisode)
                                navigationService.navigateTo(NavigationService.Detail)
                                pendingContentId = ""
                                pendingType = ""
                                pendingAddonId = ""
                                pendingSeason = 0
                                pendingEpisode = 0
                            }
                        }
                    }
                    
                    onStatusChanged: {
                        if (status === Loader.Ready && item && pendingContentId && pendingType) {
                            item.loadDetails(pendingContentId, pendingType, pendingAddonId, pendingSeason, pendingEpisode)
                            navigationService.navigateTo(NavigationService.Detail)
                            pendingContentId = ""
                            pendingType = ""
                            pendingAddonId = ""
                            pendingSeason = 0
                            pendingEpisode = 0
                        }
                    }
                }
                
                Connections {
                    target: detailLoader.item
                    function onPlayRequested(streamUrl, contentData) {
                        navigationService.navigateToPlayer(streamUrl, contentData || ({}))
                    }
                }
                
                // Video Player Screen
                Loader {
                    id: videoPlayerLoader
                    active: false
                    source: "qrc:/qml/screens/VideoPlayerScreen.qml"
                    
                    property string pendingStreamUrl: ""
                    property var pendingContentData: ({})
                    
                    onLoaded: {
                        if (item && pendingStreamUrl) {
                            // Removed debug log - unnecessary
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
                            navigationService.navigateTo(NavigationService.Player)
                            pendingStreamUrl = ""
                            pendingContentData = {}
                        }
                    }
                    
                    Connections {
                        target: videoPlayerLoader.item
                        function onGoBackRequested() {
                            // Stop video playback before leaving
                            if (videoPlayerLoader.item && videoPlayerLoader.item.videoPlayer) {
                                videoPlayerLoader.item.videoPlayer.stop()
                            }
                            // Refresh home page to update "Continue Watching" after watching content
                            if (homeLoader.item) {
                                homeLoader.item.loadCatalogs()
                            }
                            navigationService.navigateBack()
                        }
                        
                        function onToggleFullscreenRequested() {
                            // Removed debug log - unnecessary
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

