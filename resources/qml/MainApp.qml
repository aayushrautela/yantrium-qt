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

