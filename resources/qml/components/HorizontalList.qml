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
                
                delegate: ContentCard {
                    posterUrl: model.posterUrl || model.poster || ""
                    title: model.title || ""
                    year: model.year || 0
                    rating: model.rating || ""
                    progress: model.progressPercent || model.progress || 0
                    badgeText: model.badgeText || ""
                    isHighlighted: model.isHighlighted || false
                    
                    onClicked: {
                        // Emit signal or handle click
                        console.log("Clicked on:", model.title)
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

