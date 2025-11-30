import QtQuick
import QtQuick.Controls

Button {
    id: root
    
    // Customizable properties
    property color buttonColor: AppStyle.primary
    property color textColor: AppStyle.onPrimary
    property int buttonRadius: AppStyle.radiusMedium
    property bool isFlat: false
    
    background: Rectangle {
        color: isFlat ? "transparent" : (root.down ? Qt.darker(buttonColor, 1.2) : buttonColor)
        radius: buttonRadius
        border.width: isFlat ? 1 : 0
        border.color: buttonColor
        
        Behavior on color {
            ColorAnimation { duration: 150 }
        }
    }
    
    contentItem: Text {
        text: root.text
        color: isFlat ? buttonColor : textColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font: AppStyle.defaultFont
    }
}

