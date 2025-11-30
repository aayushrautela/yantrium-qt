pragma Singleton
import QtQuick

QtObject {
    id: style

    // Dark Theme Colors
    readonly property color background: "#1e1e1e"
    readonly property color surface: "#2d2d2d"
    readonly property color surfaceVariant: "#3d3d3d"
    readonly property color primary: "#bb86fc"
    readonly property color primaryVariant: "#985eff"
    readonly property color secondary: "#03dac6"
    readonly property color error: "#cf6679"
    
    readonly property color onBackground: "#ffffff"
    readonly property color onSurface: "#e0e0e0"
    readonly property color onPrimary: "#000000"
    readonly property color onSecondary: "#000000"
    
    // Spacing
    readonly property int spacingSmall: 8
    readonly property int spacingMedium: 16
    readonly property int spacingLarge: 24
    
    // Border Radius
    readonly property int radiusSmall: 4
    readonly property int radiusMedium: 8
    readonly property int radiusLarge: 16
    
    // Typography
    readonly property font defaultFont: Qt.font({
        family: "Roboto",
        pointSize: 14,
        weight: Font.Normal
    })
    
    readonly property font headingFont: Qt.font({
        family: "Roboto",
        pointSize: 20,
        weight: Font.Bold
    })
}

