import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    
    property int currentIndex: 0
    signal tabClicked(int index)
    signal searchClicked(string query)
    
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
            console.warn("[NavigationBar] ===== SEARCH REQUESTED =====")
            console.warn("[NavigationBar] Query:", query)
            root.searchClicked(query)
        }
    }
}

