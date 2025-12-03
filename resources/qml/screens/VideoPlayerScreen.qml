import QtQuick
import QtQuick.Controls
import Yantrium.Components 1.0

Item {
    id: root
    
    // Expose videoPlayer for external access
    property alias videoPlayer: videoPlayer
    
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
    
    // Overlay: Create a simple Rectangle overlay (color #CC18181B) that contains Play/Pause buttons and a Slider for seeking.
    Rectangle {
        id: overlay
        anchors.fill: parent
        color: "#CC18181B"
        visible: false
        
        // Show overlay on mouse hover or when controls are needed
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onEntered: overlay.visible = true
            onExited: {
                // Keep overlay visible if video is paused
                if (!videoPlayer.isPlaying) {
                    overlay.visible = true
                } else {
                    // Hide after 3 seconds if playing
                    hideTimer.restart()
                }
            }
            
            Timer {
                id: hideTimer
                interval: 3000
                onTriggered: {
                    if (videoPlayer.isPlaying) {
                        overlay.visible = false
                    }
                }
            }
        }
        
        Column {
            anchors.centerIn: parent
            spacing: 20
            width: parent.width * 0.6
            
            // Play/Pause Button
            Button {
                id: playPauseButton
                anchors.horizontalCenter: parent.horizontalCenter
                width: 80
                height: 80
                
                // Binding: Bind the button text to VideoPlayer.isPlaying
                text: videoPlayer.isPlaying ? "⏸" : "▶"
                font.pixelSize: 32
                
                // Logic: Ensure clicking the button calls the C++ play() or pause() methods
                onClicked: {
                    if (videoPlayer.isPlaying) {
                        videoPlayer.pause()
                    } else {
                        videoPlayer.play()
                    }
                }
                
                background: Rectangle {
                    color: playPauseButton.pressed ? "#AAFFFFFF" : "#CCFFFFFF"
                    radius: 40
                }
            }
            
            // Slider for seeking
            Slider {
                id: seekSlider
                width: parent.width
                anchors.horizontalCenter: parent.horizontalCenter
                
                // Binding: Bind the Slider value to VideoPlayer.position
                from: 0
                to: videoPlayer.duration > 0 ? videoPlayer.duration : 1
                value: videoPlayer.position
                
                onMoved: {
                    // Seek to the new position
                    videoPlayer.seek(Math.round(value))
                }
                
                background: Rectangle {
                    x: seekSlider.leftPadding
                    y: seekSlider.topPadding + seekSlider.availableHeight / 2 - height / 2
                    implicitWidth: 200
                    implicitHeight: 4
                    width: seekSlider.availableWidth
                    height: implicitHeight
                    radius: 2
                    color: "#33FFFFFF"
                    
                    Rectangle {
                        width: seekSlider.visualPosition * parent.width
                        height: parent.height
                        color: "#FFFFFF"
                        radius: 2
                    }
                }
                
                handle: Rectangle {
                    x: seekSlider.leftPadding + seekSlider.visualPosition * (seekSlider.availableWidth - width)
                    y: seekSlider.topPadding + seekSlider.availableHeight / 2 - height / 2
                    implicitWidth: 16
                    implicitHeight: 16
                    radius: 8
                    color: seekSlider.pressed ? "#FFFFFF" : "#E6FFFFFF"
                    border.color: "#FFFFFF"
                    border.width: 2
                }
            }
        }
    }
    
    // Show overlay initially and when video is paused
    Component.onCompleted: {
        overlay.visible = true
    }
    
    Connections {
        target: videoPlayer
        function onIsPlayingChanged() {
            if (!videoPlayer.isPlaying) {
                overlay.visible = true
            }
        }
    }
}

