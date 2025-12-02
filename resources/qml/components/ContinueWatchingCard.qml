import QtQuick
import QtQuick.Controls
import Qt5Compat.GraphicalEffects

Item {
    id: root
    
    property string backdropUrl: ""
    property string logoUrl: ""
    property string posterUrl: ""  // Fallback for backdrop
    property string title: ""
    property string type: ""  // "movie" or "episode"
    property int season: 0
    property int episode: 0
    property string episodeTitle: ""
    property double progress: 0  // 0-100
    property bool isHovered: false
    
    signal clicked()
    
    width: 480  // 20% bigger: 400 * 1.2 = 480
    height: 270  // 20% bigger: 225 * 1.2 = 270 (Landscape aspect ratio)
    
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onEntered: isHovered = true
        onExited: isHovered = false
        onClicked: root.clicked()
    }
    
    // Rounded backdrop image container
    Item {
        id: imageContainer
        anchors.fill: parent
        
        // 1. Define the mask shape (invisible, used by layer)
        Rectangle {
            id: maskShape
            anchors.fill: parent
            radius: 8
            visible: false
        }

        // 2. Enable Layering and set the OpacityMask
        layer.enabled: true
        layer.effect: OpacityMask {
            maskSource: maskShape
        }

        // Background placeholder
        Rectangle {
            anchors.fill: parent
            color: "#2a2a2a"
            radius: 8
        }
        
        // Backdrop Image (fallback to poster if backdrop not available)
        Image {
            id: backdropImage
            anchors.fill: parent
            source: root.backdropUrl || root.posterUrl || ""
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            smooth: true
            
            // Scale on hover
            scale: isHovered ? 1.1 : 1.0
            
            Behavior on scale { 
                NumberAnimation { duration: 300; easing.type: Easing.OutQuad } 
            }
            
        }

        // Loading/Error state with debug info
        Rectangle {
            anchors.fill: parent
            color: "#80000000"
            visible: backdropImage.status !== Image.Ready || root.backdropUrl === ""
            z: 100
            
            Column {
                anchors.centerIn: parent
                spacing: 8
                
                Text {
                    text: backdropImage.status === Image.Error ? "❌ Image Error" : 
                          backdropImage.status === Image.Loading ? "⏳ Loading..." : 
                          "⚠️ No Image URL"
                    color: "#ffffff"
                    font.pixelSize: 14
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                }
                
                Text {
                    text: "Backdrop: " + (root.backdropUrl || "empty")
                    color: "#cccccc"
                    font.pixelSize: 10
                    width: 350
                    elide: Text.ElideMiddle
                }
                
                Text {
                    text: "Poster: " + (root.posterUrl || "empty")
                    color: "#cccccc"
                    font.pixelSize: 10
                    width: 350
                    elide: Text.ElideMiddle
                }
                
                Text {
                    text: "Logo: " + (root.logoUrl || "empty")
                    color: "#cccccc"
                    font.pixelSize: 10
                    width: 350
                    elide: Text.ElideMiddle
                }
                
                Text {
                    text: "Title: " + (root.title || "empty")
                    color: "#cccccc"
                    font.pixelSize: 10
                    width: 350
                    elide: Text.ElideMiddle
                }
            }
        }
        
        // Gradient overlay (darker bottom left)
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#00000000" }
                GradientStop { position: 0.5; color: "#20000000" }
                GradientStop { position: 1.0; color: "#80000000" }
            }
        }
        
        // Diagonal gradient from bottom-left corner
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#80000000" }
                GradientStop { position: 0.3; color: "#40000000" }
                GradientStop { position: 1.0; color: "#00000000" }
            }
        }
        
        // Logo (bottom left) - disappears on hover for shows
        Item {
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.margins: 16
            width: 180  // 50% bigger: 120 * 1.5 = 180
            height: 60  // 50% bigger: 40 * 1.5 = 60
            
            Image {
                id: logoImage
                anchors.fill: parent
                source: root.logoUrl
                fillMode: Image.PreserveAspectFit
                asynchronous: true
                visible: root.logoUrl !== "" && (root.type === "movie" || !isHovered)
                
                Behavior on visible {
                    NumberAnimation { duration: 200 }
                }
            }
        }
        
        // Episode info (appears on hover for shows only)
        Column {
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.margins: 16
            spacing: 4
            visible: root.type === "episode" && isHovered && (root.season > 0 || root.episode > 0)
            
            Behavior on visible {
                NumberAnimation { duration: 200 }
            }
            
            Text {
                text: "S" + (root.season < 10 ? "0" : "") + root.season +
                      "E" + (root.episode < 10 ? "0" : "") + root.episode
                color: "#ffffff"
                font.pixelSize: 21  // 50% bigger: 14 * 1.5 = 21
                font.bold: true
            }
            
            Text {
                width: 200
                text: root.episodeTitle || ""
                color: "#e0e0e0"
                font.pixelSize: 18  // 50% bigger: 12 * 1.5 = 18
                elide: Text.ElideRight
            }
        }
        
        // Progress bar overlay
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 4
            color: "#33000000"
            visible: root.progress > 0
            
            Rectangle {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: parent.width * (root.progress / 100)
                color: "#ffffff"
            }
        }
        
        // Border overlay (on hover)
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            radius: 8
            z: 10
            
            border.width: 3
            border.color: isHovered ? "#ffffff" : "transparent"
            
            Behavior on border.color { ColorAnimation { duration: 200 } }
        }
    }
}

