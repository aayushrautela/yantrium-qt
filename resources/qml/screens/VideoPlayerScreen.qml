import QtQuick
import QtQuick.Controls
import Yantrium.Components 1.0

Item {
    id: root
    
    // Expose videoPlayer for external access
    property alias videoPlayer: videoPlayer
    
    // Content information properties
    property string contentType: ""  // "movie" or "tv"
    property string contentTitle: ""
    property string contentDescription: ""
    property string logoUrl: ""
    property int seasonNumber: 0
    property int episodeNumber: 0
    
    // Signals
    signal goBackRequested()
    signal toggleFullscreenRequested()
    
    // Helper function to format time from milliseconds to MM:SS
    function formatTime(ms) {
        if (ms < 0) return "00:00"
        var totalSeconds = Math.floor(ms / 1000)
        var minutes = Math.floor(totalSeconds / 60)
        var seconds = totalSeconds % 60
        return (minutes < 10 ? "0" : "") + minutes + ":" + (seconds < 10 ? "0" : "") + seconds
    }
    
    // Video Element: Use the VideoPlayer item we registered. Make it fill the parent.
    // Rounded Corners: Apply layer.effect in Qt 6 to give it 24px rounded corners
    Rectangle {
        id: videoContainer
        anchors.fill: parent
        radius: 24
        color: "black"
        clip: true
        
        VideoPlayer {
            id: videoPlayer
            anchors.fill: parent
            
            // Set video source (can be set from parent)
            source: ""
            
            // Apply layer effect for additional visual polish
            layer.enabled: true
            layer.smooth: true
        }
    }
    
    // Top Bar: Shows on hover in top area
    Rectangle {
        id: topBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 80
        color: "#80000000"
        visible: topHoverArea.containsMouse
        opacity: visible ? 1 : 0
        
        Behavior on opacity {
            OpacityAnimator {
                duration: 200
            }
        }
        
        Row {
            anchors.left: parent.left
            anchors.leftMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            spacing: 16
            
            // Back button
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 40
                height: 40
                radius: 20
                color: "transparent"
                
                Text {
                    anchors.centerIn: parent
                    text: "←"
                    color: "#FFFFFF"
                    font.pixelSize: 24
                }
                
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        // Emit signal to go back
                        root.goBackRequested()
                    }
                }
            }
            
            // Content title
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: contentTitle || (contentType === "movie" ? "Movie" : "Episode")
                color: "#FFFFFF"
                font.pixelSize: 16
                font.bold: true
            }
        }
        
        // Fullscreen button
        Rectangle {
            anchors.right: parent.right
            anchors.rightMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            width: 40
            height: 40
            radius: 20
            color: "transparent"
            
            Text {
                anchors.centerIn: parent
                text: "⛶"
                color: "#FFFFFF"
                font.pixelSize: 20
            }
            
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    root.toggleFullscreenRequested()
                }
            }
        }
    }
    
    // Mouse area for top bar hover detection
    MouseArea {
        id: topHoverArea
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 150
        hoverEnabled: true
        propagateComposedEvents: true
        z: -1
    }
    
    // Center Overlay: Only shows when paused
    Rectangle {
        id: overlay
        anchors.fill: parent
        color: "#CC18181B"
        opacity: !videoPlayer.isPlaying ? 1 : 0
        visible: opacity > 0
        
        Behavior on opacity {
            OpacityAnimator {
                duration: 200
            }
        }
        
        Column {
            anchors.centerIn: parent
            spacing: 24
            width: parent.width * 0.6
            
            // Content info section
            Column {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 12
                width: parent.width
                
                // For TV shows: Season and Episode info
                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 8
                    visible: contentType === "tv" && seasonNumber > 0
                    
                    // Season badge
                    Rectangle {
                        height: 28
                        width: seasonText.width + 24
                        radius: 14
                        color: "#1E40AF"  // Blue color
                        
                        Text {
                            id: seasonText
                            anchors.centerIn: parent
                            text: "SEASON " + seasonNumber
                            color: "#FFFFFF"
                            font.pixelSize: 12
                            font.bold: true
                        }
                    }
                    
                    // Episode number
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "• Episode " + episodeNumber
                        color: "#E5E7EB"
                        font.pixelSize: 14
                    }
                }
                
                // For movies: Logo image, for TV: Episode title placeholder
                Item {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: parent.width
                    height: contentType === "movie" ? 120 : 0
                    visible: contentType === "movie" && logoUrl !== ""
                    
                    Image {
                        id: logoImage
                        anchors.centerIn: parent
                        source: logoUrl
                        fillMode: Image.PreserveAspectFit
                        width: Math.min(parent.width, 400)
                        height: Math.min(parent.height, 120)
                    }
                }
                
                // Episode/Movie title - only show if we have contentTitle and it's not a movie with logo
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: {
                        if (contentType === "movie") {
                            return contentTitle || "Movie"
                        } else {
                            return contentTitle || "Episode"
                        }
                    }
                    color: "#FFFFFF"
                    font.pixelSize: 24
                    font.bold: true
                    visible: !(contentType === "movie" && logoUrl !== "") && contentTitle !== ""
                }
                
                // Description
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: parent.width
                    text: contentDescription || ""
                    color: "#D1D5DB"
                    font.pixelSize: 14
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    maximumLineCount: 3
                    elide: Text.ElideRight
                }
            }
            
            // Resume Playing button
            Rectangle {
                id: resumeButton
                anchors.horizontalCenter: parent.horizontalCenter
                width: 200
                height: 56
                radius: 8
                color: "#FFFFFF"
                
                Row {
                    anchors.centerIn: parent
                    spacing: 8
                    
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "▶"
                        color: "#1E40AF"
                        font.pixelSize: 20
                    }
                    
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Resume Playing"
                        color: "#1E40AF"
                        font.pixelSize: 16
                        font.bold: true
                    }
                }
                
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        videoPlayer.play()
                    }
                }
            }
        }
    }
    
    // Bottom Control Bar: Only shows on mouse hover
    Rectangle {
        id: bottomControls
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 80
        color: "#80000000"
        visible: bottomHoverArea.containsMouse
        opacity: visible ? 1 : 0
        
        Behavior on opacity {
            OpacityAnimator {
                duration: 200
            }
        }
        
        // Left side controls
        Row {
            anchors.left: parent.left
            anchors.leftMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            spacing: 16
            height: parent.height
            
            // Play/Pause icon button
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 32
                height: 32
                color: "transparent"
                
                Text {
                    anchors.centerIn: parent
                    text: videoPlayer.isPlaying ? "⏸" : "▶"
                    color: "#FFFFFF"
                    font.pixelSize: 20
                }
                
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (videoPlayer.isPlaying) {
                            videoPlayer.pause()
                        } else {
                            videoPlayer.play()
                        }
                    }
                }
            }
            
            // Speaker/Volume icon
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "🔊"
                color: "#FFFFFF"
                font.pixelSize: 20
            }
            
            // Time display
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: formatTime(videoPlayer.position) + " / " + formatTime(videoPlayer.duration)
                color: "#FFFFFF"
                font.pixelSize: 14
            }
        }
        
        // Progress bar - takes most of the space
        Item {
            anchors.left: parent.left
            anchors.leftMargin: 200
            anchors.right: strIndicator.left
            anchors.rightMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            height: 4
            
            Rectangle {
                anchors.fill: parent
                radius: 2
                color: "#33FFFFFF"
                
                Rectangle {
                    width: parent.width * (videoPlayer.duration > 0 ? videoPlayer.position / videoPlayer.duration : 0)
                    height: parent.height
                    radius: 2
                    color: "#FFFFFF"
                }
            }
            
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (videoPlayer.duration > 0) {
                        var pos = mouseX / width * videoPlayer.duration
                        videoPlayer.seek(Math.round(pos))
                    }
                }
            }
        }
        
        // STR indicator
        Row {
            id: strIndicator
            anchors.right: parent.right
            anchors.rightMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8
            
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 8
                height: 8
                radius: 4
                color: "#10B981"  // Green color
            }
            
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "STR"
                color: "#FFFFFF"
                font.pixelSize: 12
            }
        }
    }
    
    // Mouse area for bottom controls hover detection
    MouseArea {
        id: bottomHoverArea
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 150
        hoverEnabled: true
        propagateComposedEvents: true
        z: -1
    }
    
    Connections {
        target: videoPlayer
        function onIsPlayingChanged() {
            // Overlay visibility is bound to !isPlaying, so this is handled automatically
        }
    }
}

