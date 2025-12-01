import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    
    property string posterUrl: ""
    property string title: ""
    property int year: 0
    property string rating: ""
    property double progress: 0  // 0-100
    property string badgeText: ""
    property bool isHighlighted: false
    
    signal clicked()
    
    width: 150
    height: 225
    radius: 8
    color: isHighlighted ? "#bb86fc" : "#1a1a1a"
    border.width: isHighlighted ? 2 : 0
    border.color: "#bb86fc"
    
    Column {
        anchors.fill: parent
        spacing: 0
        
        // Poster image
        Rectangle {
            width: parent.width
            height: parent.height - 60
            color: "#2d2d2d"
            radius: 8
            
            Image {
                id: posterImage
                anchors.fill: parent
                fillMode: Image.PreserveAspectCrop
                source: root.posterUrl
                asynchronous: true
                
                Rectangle {
                    anchors.fill: parent
                    color: "#40000000"
                    visible: posterImage.status !== Image.Ready
                    
                    Text {
                        anchors.centerIn: parent
                        text: "📽️"
                        font.pixelSize: 40
                        color: "#666666"
                    }
                }
            }
            
            // Progress bar overlay (for continue watching)
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: 4
                color: "#33000000"
                visible: root.progress > 0
                
                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: parent.width * (root.progress / 100)
                    color: "#bb86fc"
                }
            }
            
            // Badge (UP NEXT, NEW, etc.)
            Rectangle {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 8
                width: badgeText.width + 12
                height: 24
                radius: 4
                color: "#ff0000"
                visible: root.badgeText !== ""
                
                Text {
                    id: badgeText
                    anchors.centerIn: parent
                    text: root.badgeText
                    font.pixelSize: 10
                    font.bold: true
                    color: "#ffffff"
                }
            }
        }
        
        // Title and year
        Column {
            width: parent.width
            height: 60
            padding: 8
            spacing: 4
            
            Text {
                width: parent.width - 16
                text: root.title
                font.pixelSize: 14
                font.bold: true
                color: "#ffffff"
                elide: Text.ElideRight
                maximumLineCount: 2
                wrapMode: Text.WordWrap
            }
            
            Row {
                spacing: 8
                
                Text {
                    text: root.year > 0 ? root.year.toString() : ""
                    font.pixelSize: 12
                    color: "#aaaaaa"
                }
                
                Text {
                    text: root.rating !== "" ? "⭐ " + root.rating : ""
                    font.pixelSize: 12
                    color: "#ffaa00"
                    visible: root.rating !== ""
                }
            }
        }
    }
    
    MouseArea {
        anchors.fill: parent
        onClicked: root.clicked()
        cursorShape: Qt.PointingHandCursor
    }
}


