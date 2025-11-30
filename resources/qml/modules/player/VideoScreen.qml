import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../components"

Rectangle {
    id: root
    color: AppStyle.background
    
    // TODO: Connect to PlayerBridge for video playback
    
    ColumnLayout {
        anchors.fill: parent
        spacing: AppStyle.spacingMedium
        
        // Video area placeholder
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#000000"
            
            // TODO: Replace with MDK video player view
            Text {
                anchors.centerIn: parent
                text: "Video Player Area"
                color: AppStyle.onSurface
                font: AppStyle.headingFont
            }
        }
        
        // Controls overlay (when needed)
        VideoOverlay {
            Layout.fillWidth: true
            Layout.preferredHeight: 100
            visible: false  // Show/hide on hover or interaction
            
            // TODO: Add video controls here
        }
    }
}

