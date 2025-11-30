import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    
    property alias content: contentLayout.children
    
    color: Qt.rgba(0, 0, 0, 0.7)  // Semi-transparent dark overlay
    radius: AppStyle.radiusMedium
    
    // Ensure it's always on top of video
    z: 100
    
    ColumnLayout {
        id: contentLayout
        anchors.fill: parent
        anchors.margins: AppStyle.spacingMedium
        spacing: AppStyle.spacingSmall
    }
}

