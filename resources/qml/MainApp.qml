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
                        stackLayout.currentIndex = index
                    }
                }
                
                onLoaded: {
                    if (item) {
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
                                    
                                    // Activate and switch to detail screen
                                    detailLoader.active = true
                                    stackLayout.currentIndex = 3  // Switch to detail screen
                                    
                                    // Load details (will be called in onLoaded if item not ready yet)
                                    if (detailLoader.item) {
                                        detailLoader.item.loadDetails(contentId, type, addonId || "")
                                    }
                                }
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
                                    
                                    // Activate and switch to detail screen
                                    detailLoader.active = true
                                    stackLayout.currentIndex = 3  // Switch to detail screen
                                    
                                    // Load details (will be called in onLoaded if item not ready yet)
                                    if (detailLoader.item) {
                                        detailLoader.item.loadDetails(contentId, type, addonId || "")
                                    }
                                }
                            })
                        }
                    }
                }
                
                Loader {
                    source: "qrc:/qml/screens/SettingsScreen.qml"
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
                            // Connect closeRequested to go back to home
                            item.closeRequested.connect(function() {
                                stackLayout.currentIndex = 0
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
                                item.loadDetails(pendingContentId, pendingType, pendingAddonId)
                                pendingContentId = ""
                                pendingType = ""
                                pendingAddonId = ""
                            }
                        }
                    }
                }
            }
        }
    }
}

