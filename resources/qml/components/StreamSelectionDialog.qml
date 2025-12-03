import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Yantrium.Services 1.0

Dialog {
    id: root
    
    property StreamService streamService: StreamService {
        id: streamService
    }
    
    property var streams: []
    property bool isLoading: false
    
    signal streamSelected(var stream)
    
    width: 600
    height: 500
    modal: true
    anchors.centerIn: parent
    
    background: Rectangle {
        color: "#1a1a1a"
        radius: 8
        border.width: 1
        border.color: "#333333"
    }
    
    header: Item {
        height: 60
        Rectangle {
            anchors.fill: parent
            color: "#1a1a1a"
            radius: 8
        }
        Text {
            anchors.left: parent.left
            anchors.leftMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            text: "Select Stream"
            color: "#ffffff"
            font.pixelSize: 20
            font.bold: true
        }
        Button {
            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            width: 30
            height: 30
            text: "×"
            background: Rectangle {
                color: parent.hovered ? "#333333" : "transparent"
                radius: 4
            }
            contentItem: Text {
                text: parent.text
                color: "#ffffff"
                font.pixelSize: 18
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            onClicked: root.close()
        }
    }
    
    contentItem: Item {
        ScrollView {
            anchors.fill: parent
            clip: true
            
            ListView {
                id: streamList
                anchors.fill: parent
                model: root.streams
                spacing: 8
                
                delegate: Rectangle {
                    width: streamList.width
                    height: streamItemColumn.implicitHeight + 20
                    color: mouseArea.containsMouse ? "#2a2a2a" : "#1a1a1a"
                    radius: 4
                    border.width: 1
                    border.color: "#333333"
                    
                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            root.streamSelected(modelData)
                            root.close()
                        }
                    }
                    
                    Column {
                        id: streamItemColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 10
                        spacing: 6
                        
                        Text {
                            text: modelData.name || modelData.title || "Unnamed Stream"
                            color: "#ffffff"
                            font.pixelSize: 16
                            font.bold: true
                            width: parent.width
                            elide: Text.ElideRight
                        }
                        
                        Text {
                            text: modelData.description || ""
                            color: "#aaaaaa"
                            font.pixelSize: 14
                            width: parent.width
                            wrapMode: Text.Wrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                            visible: text !== ""
                        }
                        
                        Row {
                            spacing: 10
                            visible: modelData.quality || modelData.addonName
                            
                            Text {
                                text: modelData.quality || ""
                                color: "#888888"
                                font.pixelSize: 12
                                visible: text !== ""
                            }
                            
                            Text {
                                text: modelData.addonName || ""
                                color: "#666666"
                                font.pixelSize: 12
                                visible: text !== ""
                            }
                        }
                    }
                }
            }
        }
    }
    
    footer: Item {
        height: 50
        Rectangle {
            anchors.fill: parent
            color: "#1a1a1a"
        }
        Text {
            anchors.centerIn: parent
            text: root.isLoading ? "Loading streams..." : (root.streams.length > 0 ? `${root.streams.length} stream(s) available` : "No streams available")
            color: "#888888"
            font.pixelSize: 14
        }
    }
    
    Connections {
        target: streamService
        function onStreamsLoaded(streams) {
            root.streams = streams
            root.isLoading = false
        }
        function onError(message) {
            root.isLoading = false
            console.error("[StreamSelectionDialog] Error:", message)
        }
    }
    
    function loadStreams(itemData, episodeId) {
        root.streams = []
        root.isLoading = true
        streamService.getStreamsForItem(itemData, episodeId || "")
    }
}

