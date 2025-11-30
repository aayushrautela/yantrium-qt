import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    
    property string backdropUrl: ""
    property string posterUrl: ""
    property string title: ""
    property string description: ""
    property var metadata: []  // Array of strings like ["2023", "PG", "Action"]
    property bool hasMultipleItems: false
    property int currentIndex: 0
    
    signal playClicked()
    signal addToLibraryClicked()
    signal previousClicked()
    signal nextClicked()
    
    width: parent.width
    height: 500
    color: "#000000"
    
    // Backdrop image
    Image {
        id: backdropImage
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
        source: root.backdropUrl
        asynchronous: true
        
        // Gradient overlay
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#00000000" }
                GradientStop { position: 0.5; color: "#80000000" }
                GradientStop { position: 1.0; color: "#ff000000" }
            }
        }
    }
    
    // Content overlay
    Row {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 40
        spacing: 30
        height: parent.height - 80
        
        // Poster
        Image {
            width: 200
            height: 300
            fillMode: Image.PreserveAspectFit
            source: root.posterUrl
            asynchronous: true
        }
        
        // Text content
        Column {
            width: parent.width - 230
            spacing: 16
            anchors.verticalCenter: parent.verticalCenter
            
            // Title
            Text {
                width: parent.width
                text: root.title
                font.pixelSize: 48
                font.bold: true
                color: "#ffffff"
                wrapMode: Text.WordWrap
            }
            
            // Metadata tags
            Row {
                spacing: 8
                Repeater {
                    model: root.metadata
                    Rectangle {
                        height: 28
                        width: metadataText.width + 16
                        radius: 4
                        color: "#40ffffff"
                        
                        Text {
                            id: metadataText
                            anchors.centerIn: parent
                            text: modelData
                            font.pixelSize: 12
                            color: "#ffffff"
                        }
                    }
                }
            }
            
            // Description
            Text {
                width: parent.width
                text: root.description
                font.pixelSize: 16
                color: "#e0e0e0"
                wrapMode: Text.WordWrap
                maximumLineCount: 4
                elide: Text.ElideRight
            }
            
            // Action buttons
            Row {
                spacing: 12
                
                Button {
                    width: 140
                    height: 44
                    text: "▶ Play Now"
                    background: Rectangle {
                        color: "#ff0000"
                        radius: 4
                    }
                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: 16
                        font.bold: true
                        color: "#ffffff"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: root.playClicked()
                }
                
                Button {
                    width: 160
                    height: 44
                    text: "+ Add to Library"
                    background: Rectangle {
                        color: "#40000000"
                        border.width: 1
                        border.color: "#ffffff"
                        radius: 4
                    }
                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: 16
                        color: "#ffffff"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: root.addToLibraryClicked()
                }
            }
        }
    }
    
    // Navigation arrows (if multiple items)
    Rectangle {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        width: 50
        height: 50
        radius: 25
        color: "#80000000"
        visible: root.hasMultipleItems
        border.width: 1
        border.color: "#ffffff"
        
        Text {
            anchors.centerIn: parent
            text: "◀"
            font.pixelSize: 24
            color: "#ffffff"
        }
        
        MouseArea {
            anchors.fill: parent
            onClicked: root.previousClicked()
        }
    }
    
    Rectangle {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        width: 50
        height: 50
        radius: 25
        color: "#80000000"
        visible: root.hasMultipleItems
        border.width: 1
        border.color: "#ffffff"
        
        Text {
            anchors.centerIn: parent
            text: "▶"
            font.pixelSize: 24
            color: "#ffffff"
        }
        
        MouseArea {
            anchors.fill: parent
            onClicked: root.nextClicked()
        }
    }
}

