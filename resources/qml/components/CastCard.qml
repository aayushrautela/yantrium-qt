import QtQuick
import QtQuick.Controls
import Qt5Compat.GraphicalEffects

Item {
    id: root
    
    property string profileImageUrl: ""
    property string actorName: ""
    property string characterName: ""
    
    // Card dimensions are set by parent, but provide defaults
    width: 240
    height: 360
    
    Column {
        anchors.fill: parent
        spacing: 12
        
        // Circular Profile Image (scales with card width)
        Item {
            width: parent.width * 0.85  // 85% of card width (larger image)
            height: width  // Keep it circular
            anchors.horizontalCenter: parent.horizontalCenter
            
            // Mask shape for circular clipping
            Rectangle {
                id: maskShape
                anchors.fill: parent
                radius: width / 2
                visible: false
            }
            
            // Enable layering and set OpacityMask
            layer.enabled: true
            layer.effect: OpacityMask {
                maskSource: maskShape
            }
            
            // Background placeholder
            Rectangle {
                anchors.fill: parent
                color: "#2a2a2a"
                radius: width / 2
            }
            
            // Profile Image
            Image {
                id: profileImage
                anchors.fill: parent
                source: root.profileImageUrl
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                smooth: true
            }
        }
        
        // Actor Name
        Text {
            width: parent.width
            text: root.actorName
            color: "#ffffff"
            font.pixelSize: Math.max(18, parent.width * 0.075)  // Scale with card width, increased size
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            maximumLineCount: 2
            wrapMode: Text.WordWrap
        }
        
        // Character Name
        Text {
            width: parent.width
            text: root.characterName
            color: "#aaaaaa"
            font.pixelSize: Math.max(16, parent.width * 0.067)  // Scale with card width, increased size
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            maximumLineCount: 2
            wrapMode: Text.WordWrap
        }
    }
}

