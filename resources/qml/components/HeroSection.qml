import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    
    property string backdropUrl: ""
    property string title: ""
    property string logoUrl: ""
    property string description: ""
    property var metadata: []  // Array of strings like ["2023", "PG", "Action"]
    property bool hasMultipleItems: false
    property int currentIndex: 0
    
    property string nextBackdropUrl: ""
    property bool isAnimating: false
    
    function updateBackdrop(url, animate) {
        if (animate && url !== backdropUrl && !isAnimating) {
            nextBackdropUrl = url
            isAnimating = true
            // Start with next image off-screen to the right
            nextBackdropImage.x = backdropContainer.width
            slideAnimation.start()
        } else {
            backdropUrl = url
            nextBackdropUrl = ""
            isAnimating = false
        }
    }
    
    signal playClicked()
    signal addToLibraryClicked()
    signal previousClicked()
    signal nextClicked()
    
    width: parent.width
    height: 650
    color: "#000000"
    
    // Backdrop images container for slide animation
    Item {
        id: backdropContainer
        anchors.fill: parent
        clip: true
        
        // Current backdrop image
        Image {
            id: backdropImage
            anchors.fill: parent
            fillMode: Image.PreserveAspectCrop
            source: root.backdropUrl
            asynchronous: true
            x: 0
            
            // Vertical gradient overlay (top to bottom)
            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#00000000" }
                    GradientStop { position: 0.5; color: "#8009090b" }
                    GradientStop { position: 1.0; color: "#ff09090b" }
                }
            }
            
            // Horizontal gradient overlay (left to middle)
            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: "#ff09090b" }
                    GradientStop { position: 0.5; color: "#0009090b" }
                }
            }
        }
        
        // Next backdrop image (slides in)
        Image {
            id: nextBackdropImage
            anchors.fill: parent
            fillMode: Image.PreserveAspectCrop
            source: root.nextBackdropUrl
            asynchronous: true
            visible: root.isAnimating
            x: root.isAnimating ? 0 : parent.width
            
            // Vertical gradient overlay (top to bottom)
            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#00000000" }
                    GradientStop { position: 0.5; color: "#8009090b" }
                    GradientStop { position: 1.0; color: "#ff09090b" }
                }
            }
            
            // Horizontal gradient overlay (left to middle)
            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: "#ff09090b" }
                    GradientStop { position: 0.5; color: "#0009090b" }
                }
            }
        }
    }
    
    // Slide animation
    ParallelAnimation {
        id: slideAnimation
        
        NumberAnimation {
            target: backdropImage
            property: "x"
            from: 0
            to: -backdropContainer.width
            duration: 800
            easing.type: Easing.InOutQuad
        }
        
        NumberAnimation {
            target: nextBackdropImage
            property: "x"
            from: backdropContainer.width
            to: 0
            duration: 800
            easing.type: Easing.InOutQuad
        }
        
        onFinished: {
            // Swap images after animation
            root.backdropUrl = root.nextBackdropUrl
            root.nextBackdropUrl = ""
            root.isAnimating = false
            backdropImage.x = 0
            nextBackdropImage.x = backdropContainer.width
        }
    }
    
    // Content overlay
    Column {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 60
        anchors.rightMargin: 40
        anchors.bottomMargin: 80
        spacing: 16
        
        // Logo
        Item {
            width: Math.min(parent.width, 550)
            height: 160
            
            Image {
                anchors.left: parent.left
                width: Math.min(parent.width, 550)
                height: 160
                fillMode: Image.PreserveAspectFit
                source: root.logoUrl
                asynchronous: true
                visible: root.logoUrl !== ""
                
                // Fallback to title if logo is not available
                Text {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.title
                    font.pixelSize: 48
                    font.bold: true
                    color: "#ffffff"
                    wrapMode: Text.WordWrap
                    visible: parent.status === Image.Error || root.logoUrl === ""
                }
            }
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
            width: Math.min(parent.width * 0.75, 600)
            text: root.description
            font.pixelSize: 16
            color: "#e0e0e0"
            wrapMode: Text.WordWrap
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


