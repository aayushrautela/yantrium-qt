import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    
    property int currentIndex: 0
    property bool showBackButton: false
    signal tabClicked(int index)
    signal searchClicked(string query)
    signal backClicked()
    
    onTabClicked: function(index) {
        currentIndex = index
    }
    
    width: parent.width
    height: 60
    color: "#09090b"
    
    Row {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 20
        spacing: 40
        
        // Back Button (with reserved space to prevent content shifting)
        Item {
            width: 48
            height: 48
            anchors.verticalCenter: parent.verticalCenter
            
            property bool isHovered: false
            
            Image {
                id: backIcon
                anchors.centerIn: parent
                width: 24
                height: 24
                source: "qrc:/assets/icons/back.svg"
                sourceSize.width: 48
                sourceSize.height: 48
                fillMode: Image.PreserveAspectFit
                smooth: true
                antialiasing: true
                opacity: parent.isHovered ? 1.0 : (root.showBackButton ? 0.7 : 0.0)
                visible: root.showBackButton
                Behavior on opacity { NumberAnimation { duration: 200 } }
            }
            
            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                enabled: root.showBackButton
                onEntered: parent.isHovered = true
                onExited: parent.isHovered = false
                onClicked: {
                    if (root.showBackButton) {
                        root.backClicked()
                    }
                }
            }
        }
        
        // Logo
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: "YANTRIUM"
            font.pixelSize: 24
            font.bold: true
            color: "#ffffff"
        }
        
        // Navigation tabs
        Row {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8
            
            Repeater {
                model: ["Home", "Library", "Settings"]
                
                Rectangle {
                    width: tabText.width + 20
                    height: 36
                    radius: 4
                    color: root.currentIndex === index ? "#ffffff" : "transparent"
                    
                    Text {
                        id: tabText
                        anchors.centerIn: parent
                        text: modelData
                        font.pixelSize: 14
                        font.bold: root.currentIndex === index
                        color: root.currentIndex === index ? "#000000" : "#ffffff"
                    }
                    
                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.tabClicked(index)
                    }
                }
            }
        }
    }
    
    // Search bar (right side)
    SearchBar {
        id: searchBar
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.rightMargin: 20
        
        onSearchRequested: function(query) {
            // Logging handled by MainApp
            root.searchClicked(query)
        }
    }
}

