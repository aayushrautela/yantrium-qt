import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

Rectangle {
    id: root
    
    property string title: ""
    property string icon: ""
    property var model: null
    property int itemWidth: 150
    property int itemHeight: 225
    property string addonId: ""  // Addon ID for this section
    
    signal itemClicked(string contentId, string type, string addonId)
    
    width: parent.width
    height: (root.title === "Continue Watching" ? 270 : itemHeight) + 60  // Content height + title height (270 for continue watching cards)
    
    color: "transparent"
    
    Column {
        anchors.fill: parent
        spacing: 12
        
        // Section title
        Row {
            width: parent.width
            height: 32
            spacing: 8
            leftPadding: 50
            
            Text {
                text: root.icon !== "" ? root.icon + " " : ""
                font.pixelSize: 20
                color: "#ffffff"
            }
            
            Text {
                text: root.title
                font.pixelSize: 18
                font.bold: true
                color: "#ffffff"
            }
        }
        
        // Horizontal scrolling list
        Item {
            width: parent.width - 100  // Reduce width to leave 50px on each side
            height: itemHeight
            anchors.horizontalCenter: parent.horizontalCenter
            
            ListView {
                id: listView
                anchors.fill: parent
                orientation: ListView.Horizontal
                spacing: 12
                leftMargin: 0
                rightMargin: 0
                clip: true
                model: root.model
                
                PropertyAnimation {
                    id: scrollAnimation
                    target: listView
                    property: "contentX"
                    duration: 300
                    easing.type: Easing.OutCubic
                }
                
                delegate: Loader {
                    width: root.title === "Continue Watching" ? 480 : root.itemWidth  // 20% bigger: 400 * 1.2 = 480
                    height: root.title === "Continue Watching" ? 270 : root.itemHeight  // 20% bigger: 225 * 1.2 = 270
                    
                    source: root.title === "Continue Watching" 
                        ? "qrc:/qml/components/ContinueWatchingCard.qml"
                        : "qrc:/qml/components/ContentCard.qml"
                    
                    onLoaded: {
                        if (!item) return
                        
                        if (root.title === "Continue Watching") {
                            // Use property bindings for reactive updates
                            item.backdropUrl = Qt.binding(function() { return model.backdropUrl || "" })
                            item.logoUrl = Qt.binding(function() { return model.logoUrl || "" })
                            item.posterUrl = Qt.binding(function() { return model.posterUrl || "" })
                            item.title = Qt.binding(function() { return model.title || "" })
                            item.type = Qt.binding(function() { return model.type || "" })
                            item.season = Qt.binding(function() { return model.season || 0 })
                            item.episode = Qt.binding(function() { return model.episode || 0 })
                            item.episodeTitle = Qt.binding(function() { return model.episodeTitle || "" })
                            item.progress = Qt.binding(function() { return model.progress || model.progressPercent || 0 })
                            
                            
                            item.clicked.connect(function() {
                                // Get content ID - prefer TMDB ID if available (1 API call), otherwise IMDB ID (2 API calls)
                                var contentId = ""
                                if (model.tmdbId && model.tmdbId !== "") {
                                    contentId = model.tmdbId
                                } else if (model.imdbId && model.imdbId.startsWith("tt")) {
                                    contentId = model.imdbId
                                } else if (model.id && model.id !== "") {
                                    if (model.id.startsWith("tt")) {
                                        contentId = model.id
                                    } else if (model.id.startsWith("tmdb:")) {
                                        contentId = model.id.substring(5)
                                    } else if (!isNaN(model.id) && parseInt(model.id) > 0) {
                                        contentId = model.id
                                    }
                                }
                                root.itemClicked(contentId, model.type || "", model.addonId || root.addonId || "")
                            })
                        } else {
                            item.posterUrl = Qt.binding(function() { return model.posterUrl || model.poster || "" })
                            item.title = Qt.binding(function() { return model.title || "" })
                            item.year = Qt.binding(function() { return model.year || 0 })
                            item.rating = Qt.binding(function() { return model.rating || "" })
                            item.progress = Qt.binding(function() { return model.progressPercent || model.progress || 0 })
                            item.badgeText = Qt.binding(function() { return model.badgeText || "" })
                            item.isHighlighted = Qt.binding(function() { return model.isHighlighted || false })
                            item.clicked.connect(function() {
                                // Get content ID - prefer TMDB ID if available (1 API call), otherwise IMDB ID (2 API calls)
                                var contentId = ""
                                if (model.tmdbId && model.tmdbId !== "") {
                                    contentId = model.tmdbId
                                } else if (model.imdbId && model.imdbId.startsWith("tt")) {
                                    contentId = model.imdbId
                                } else if (model.id && model.id !== "") {
                                    if (model.id.startsWith("tt")) {
                                        contentId = model.id
                                    } else if (model.id.startsWith("tmdb:")) {
                                        contentId = model.id.substring(5)
                                    } else if (!isNaN(model.id) && parseInt(model.id) > 0) {
                                        contentId = model.id
                                    }
                                }
                                root.itemClicked(contentId, model.type || "", model.addonId || root.addonId || "")
                            })
                        }
                    }
                }
            }
            
            // Left arrow
            Item {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                width: 48
                height: 48
                visible: listView.contentX > 0
                
                property bool isHovered: false
                
                Item {
                    anchors.centerIn: parent
                    width: 48
                    height: 48
                    opacity: parent.isHovered ? 1.0 : 0.4
                    Behavior on opacity { NumberAnimation { duration: 200 } }
                    
                    Image {
                        id: leftArrowIcon
                        anchors.centerIn: parent
                        
                        source: "qrc:/assets/icons/left_catalog.svg"
                        
                        // Large source + mipmap ensures smooth downscaling
                        sourceSize.width: 128
                        sourceSize.height: 128
                        mipmap: true
                        smooth: true
                        antialiasing: true
                        
                        width: 48
                        height: 48
                        fillMode: Image.PreserveAspectFit
                        visible: false
                    }
                    
                    ColorOverlay {
                        anchors.fill: leftArrowIcon
                        source: leftArrowIcon
                        color: "#ffffff"
                        cached: true
                        antialiasing: true
                    }
                }
                
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: parent.isHovered = true
                    onExited: parent.isHovered = false
                    onClicked: {
                        var targetX = Math.max(0, listView.contentX - listView.width * 0.8)
                        scrollAnimation.to = targetX
                        scrollAnimation.start()
                    }
                }
            }
            
            // Right arrow
            Item {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: 48
                height: 48
                visible: listView.contentX < listView.contentWidth - listView.width
                
                property bool isHovered: false
                
                Item {
                    anchors.centerIn: parent
                    width: 48
                    height: 48
                    opacity: parent.isHovered ? 1.0 : 0.4
                    Behavior on opacity { NumberAnimation { duration: 200 } }
                    
                    Image {
                        id: rightArrowIcon
                        anchors.centerIn: parent
                        
                        source: "qrc:/assets/icons/right_catalog.svg"
                        
                        // Large source + mipmap ensures smooth downscaling
                        sourceSize.width: 128
                        sourceSize.height: 128
                        mipmap: true
                        smooth: true
                        antialiasing: true
                        
                        width: 48
                        height: 48
                        fillMode: Image.PreserveAspectFit
                        visible: false
                    }
                    
                    ColorOverlay {
                        anchors.fill: rightArrowIcon
                        source: rightArrowIcon
                        color: "#ffffff"
                        cached: true
                        antialiasing: true
                    }
                }
                
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: parent.isHovered = true
                    onExited: parent.isHovered = false
                    onClicked: {
                        var targetX = Math.min(
                            listView.contentWidth - listView.width,
                            listView.contentX + listView.width * 0.8
                        )
                        scrollAnimation.to = targetX
                        scrollAnimation.start()
                    }
                }
            }
        }
    }
}

