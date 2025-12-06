import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    
    // --- API Properties ---
    property string title: ""
    property string description: ""
    property string logoUrl: ""
    property var metadata: [] // Array of strings e.g. ["2024", "PG-13"]
    
    // --- Signals ---
    signal playClicked()
    signal moreInfoClicked()
    
    // --- Internal Logic ---
    // We use two images to ping-pong: one is visible, one waits to slide in.
    property bool useImageA: true 
    
    function updateBackdrop(url, direction) {
        // direction: 1 = Next (Slide from Right), -1 = Prev (Slide from Left), 0 = Instant
        
        var targetImage = useImageA ? bgImageB : bgImageA
        var currentImage = useImageA ? bgImageA : bgImageB
        
        // 1. Setup the "Incoming" image
        targetImage.source = url
        targetImage.visible = true
        
        if (direction === 0) {
            // No Animation (Instant)
            targetImage.x = 0
            currentImage.visible = false
            useImageA = !useImageA
            return
        }
        
        // 2. Position "Incoming" image based on direction
        // If Next (1): enter from Right (width). If Prev (-1): enter from Left (-width)
        targetImage.x = (direction > 0) ? root.width : -root.width
        
        // 3. Define Animation Targets
        animOut.target = currentImage
        animOut.to = (direction > 0) ? -root.width : root.width // Push existing away
        animIn.target = targetImage
        animIn.to = 0 // Bring new one to center
        
        // 4. Run
        slideAnim.start()
        
        // 5. Swap logic (happens after animation starts, state update)
        useImageA = !useImageA
    }
    
    // --- Animations ---
    ParallelAnimation {
        id: slideAnim
        
        NumberAnimation {
            id: animOut
            property: "x"
            duration: 500
            easing.type: Easing.OutQuart
        }
        NumberAnimation {
            id: animIn
            property: "x"
            duration: 500
            easing.type: Easing.OutQuart
        }
        
        onFinished: {
            // Hide the image that slid out
            if (useImageA) {
                bgImageB.visible = false
                bgImageB.x = root.width
            } else {
                bgImageA.visible = false
                bgImageA.x = -root.width
            }
        }
    }
    
    width: parent.width
    height: 650
    
    // --- Visuals ---
    
    // 1. Background Layer
    Item {
        id: backdropContainer
        anchors.fill: parent
        clip: true // Essential to crop the image sliding out
        
        Image {
            id: bgImageA
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: parent.width
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            x: 0
        }
        
        Image {
            id: bgImageB
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: parent.width
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
            visible: false
            x: parent.width 
        }
        
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
    
    // 2. Content Layer (Static - Updates instantly)
    Column {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.leftMargin: 120
        anchors.rightMargin: 40
        anchors.bottomMargin: 80
        spacing: 24
        
        // Logo or Text Title
        Item {
            width: Math.min(parent.width, 550)
            height: 160
            
        Image {
                anchors.left: parent.left
                width: Math.min(parent.width, 550)
                height: 160
                fillMode: Image.PreserveAspectFit
                horizontalAlignment: Image.AlignLeft
                verticalAlignment: Image.AlignVCenter
                source: root.logoUrl
                visible: root.logoUrl !== ""
                asynchronous: true
                
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
            
            // Metadata as plain text
            Text {
                text: {
                    if (root.metadata.length === 0) return "";

                    // Format: rating • year • runtime • genre1 genre2 •
                    var result = "";
                    if (root.metadata.length >= 1) result += root.metadata[0]; // rating
                    if (root.metadata.length >= 2) result += " • " + root.metadata[1]; // year
                    if (root.metadata.length >= 3) result += " • " + root.metadata[2]; // runtime

                    // Add genres separated by spaces, max 2 genres
                    if (root.metadata.length >= 4) {
                        result += " • ";
                        var genres = [];
                        for (var i = 3; i < Math.min(root.metadata.length, 5); i++) { // genres start at index 3, max 2
                            genres.push(root.metadata[i]);
                        }
                        result += genres.join(" ");
                    }

                    return result;
                }
                font.pixelSize: 14
                color: "#ffffff"
                visible: root.metadata.length > 0
            }
            
            // Description
            Text {
            width: Math.min(parent.width * 0.75, 600)
                text: root.description
            color: "#e0e0e0"
                font.pixelSize: 18
                wrapMode: Text.WordWrap
                maximumLineCount: 3
                elide: Text.ElideRight
            }
            
        // Buttons
            Row {
                spacing: 12
                
                Button {
                    width: 160
                    height: 52
                    text: "▶ Play Now"
                    scale: hovered ? 1.05 : 1.0
                    Behavior on scale { NumberAnimation { duration: 150; easing.type: Easing.OutQuad } }
                    background: Rectangle {
                        color: "#ffffff"
                        radius: 26 // Pill shape (half of height)
                    }
                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: 18
                        font.bold: true
                        color: "#000000"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: root.playClicked()
                }
                
                Button {
                    width: 160
                    height: 52
                    text: "More Info"
                    scale: hovered ? 1.05 : 1.0
                    Behavior on scale { NumberAnimation { duration: 150; easing.type: Easing.OutQuad } }
                    background: Rectangle {
                        color: "#40000000"
                        border.width: 1
                        border.color: "#ffffff"
                        radius: 26
                    }
                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: 16
                        color: "#ffffff"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: root.moreInfoClicked()
                }
            }
        }
    }
