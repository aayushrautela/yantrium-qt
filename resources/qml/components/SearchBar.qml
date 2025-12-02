import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    
    property string placeholderText: "Titles, people, genres"
    signal searchRequested(string query)
    
    width: 300
    height: 36
    radius: 18
    color: "#2d2d2d"
    border.width: 1
    border.color: "#3d3d3d"
    
    Row {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 8
        
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: "🔍"
            font.pixelSize: 16
            color: "#aaaaaa"
        }
        
        TextField {
            id: searchField
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width - 60
            height: parent.height - 8
            background: Rectangle {
                color: "transparent"
            }
            placeholderText: root.placeholderText
            placeholderTextColor: "#666666"
            color: "#ffffff"
            font.pixelSize: 14
            
            onTextChanged: {
                if (text.length > 0) {
                    clearButton.visible = true
                } else {
                    clearButton.visible = false
                }
            }
            
            onAccepted: {
                root.searchRequested(text)
            }
        }
        
        Rectangle {
            id: clearButton
            anchors.verticalCenter: parent.verticalCenter
            width: 20
            height: 20
            radius: 10
            color: "#666666"
            visible: false
            
            Text {
                anchors.centerIn: parent
                text: "✕"
                font.pixelSize: 12
                color: "#ffffff"
            }
            
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    searchField.text = ""
                    clearButton.visible = false
                }
            }
        }
    }
}




