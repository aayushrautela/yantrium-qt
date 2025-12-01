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
                        console.log("[MainApp] HomeScreen Loader loaded")
                        console.log("[MainApp] homeLoader.item:", item)
                        if (item) {
                            console.log("[MainApp] HomeScreen item exists, calling loadCatalogs()")
                            item.loadCatalogs()
                        } else {
                            console.error("[MainApp] ERROR: HomeScreen item is null!")
                        }
                    }
                    
                    onStatusChanged: {
                        console.log("[MainApp] HomeScreen Loader status changed:", status)
                        if (status === Loader.Error) {
                            console.error("[MainApp] ERROR: Failed to load HomeScreen:", source)
                        }
                    }
                    
                    // Reload when switching to home tab
                    Connections {
                        target: stackLayout
                        function onCurrentIndexChanged() {
                            console.log("[MainApp] StackLayout currentIndex changed to:", stackLayout.currentIndex)
                            if (stackLayout.currentIndex === 0 && homeLoader.item) {
                                console.log("[MainApp] Switched to home tab, reloading catalogs")
                                homeLoader.item.loadCatalogs()
                            }
                        }
                    }
                }
                
                Item {
                    // Library screen placeholder
                    Rectangle {
                        anchors.fill: parent
                        color: "#09090b"
                        
                        Text {
                            anchors.centerIn: parent
                            text: "Library Screen\n(Coming Soon)"
                            font.pixelSize: 24
                            color: "#aaaaaa"
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }
                
                Loader {
                    source: "qrc:/qml/screens/SettingsScreen.qml"
                }
            }
        }
    }
}

