import QtQuick
import QtQuick.Controls
import Yantrium.Components 1.0

ApplicationWindow {
    id: root
    width: 1000
    height: 700
    visible: true
    title: "Yantrium - Test Launcher"

    Loader {
        id: screenLoader
        anchors.fill: parent
        source: "qrc:/qml/screens/AddonTestScreen.qml"
    }
}

