import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    
    property alias content: contentItem.children
    property int padding: AppStyle.spacingMedium
    
    color: AppStyle.surface
    radius: AppStyle.radiusMedium
    
    border.width: 1
    border.color: AppStyle.surfaceVariant
    
    Column {
        id: contentItem
        anchors.fill: parent
        anchors.margins: root.padding
        spacing: AppStyle.spacingSmall
    }
}

