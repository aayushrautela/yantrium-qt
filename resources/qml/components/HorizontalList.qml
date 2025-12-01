import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    
    property string title: ""
    property string icon: ""
    property var model: null
    property int itemWidth: 150
    property int itemHeight: 225
    
    width: parent.width
    height: itemHeight + 60  // Content height + title height
    
    color: "transparent"
    
    Column {
        anchors.fill: parent
        spacing: 12
        
        // Section title
        Row {
            width: parent.width
            height: 32
            spacing: 8
            leftPadding: 20
            
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
            width: parent.width
            height: itemHeight
            
            ListView {
                id: listView
                anchors.fill: parent
                orientation: ListView.Horizontal
                spacing: 12
                leftMargin: 20
                rightMargin: 20
                model: root.model
                
                delegate: Loader {
                    width: root.title === "Continue Watching" ? 400 : root.itemWidth
                    height: root.title === "Continue Watching" ? 225 : root.itemHeight
                    
                    source: root.title === "Continue Watching" 
                        ? "qrc:/qml/components/ContinueWatchingCard.qml"
                        : "qrc:/qml/components/ContentCard.qml"
                    
                    onLoaded: {
                        if (!item) return
                        
                        if (root.title === "Continue Watching") {
                            item.backdropUrl = model.backdropUrl || ""
                            item.logoUrl = model.logoUrl || ""
                            item.title = model.title || ""
                            item.type = model.type || ""
                            item.season = model.season || 0
                            item.episode = model.episode || 0
                            item.episodeTitle = model.episodeTitle || ""
                            item.progress = model.progress || model.progressPercent || 0
                            item.clicked.connect(function() {
                                console.log("Clicked on:", model.title)
                            })
                        } else {
                            item.posterUrl = model.posterUrl || model.poster || ""
                            item.title = model.title || ""
                            item.year = model.year || 0
                            item.rating = model.rating || ""
                            item.progress = model.progressPercent || model.progress || 0
                            item.badgeText = model.badgeText || ""
                            item.isHighlighted = model.isHighlighted || false
                            item.clicked.connect(function() {
                                console.log("Clicked on:", model.title)
                            })
                        }
                    }
                }
                
                // Left arrow
                Rectangle {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    width: 40
                    height: 40
                    radius: 20
                    color: "#80000000"
                    visible: listView.contentX > 0
                    
                    Text {
                        anchors.centerIn: parent
                        text: "◀"
                        font.pixelSize: 20
                        color: "#ffffff"
                    }
                    
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            listView.contentX = Math.max(0, listView.contentX - listView.width * 0.8)
                        }
                    }
                }
                
                // Right arrow
                Rectangle {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: 40
                    height: 40
                    radius: 20
                    color: "#80000000"
                    visible: listView.contentX < listView.contentWidth - listView.width
                    
                    Text {
                        anchors.centerIn: parent
                        text: "▶"
                        font.pixelSize: 20
                        color: "#ffffff"
                    }
                    
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            listView.contentX = Math.min(
                                listView.contentWidth - listView.width,
                                listView.contentX + listView.width * 0.8
                            )
                        }
                    }
                }
            }
        }
    }
}

