import QtQuick
import QtQuick.Controls
import Yantrium.Components 1.0
import Yantrium.Services 1.0

ApplicationWindow {
    id: root
    width: 1280
    height: 720
    visible: true
    title: "Yantrium Player"
    
    Component.onCompleted: {
        // Logging handled by services
    }

    Rectangle {
        anchors.fill: parent
        color: "#09090b"

        VideoPlayer {
            id: videoPlayer
            anchors.fill: parent
            anchors.margins: 20
            
            // Set a video source (you can change this to any video file)
            source: "https://nexus-042.ceur.tb-cdn.st/dld/40ed0067-409f-48b9-9b05-44bcc0ab35e8?token=9c809365-b77d-43b0-9f51-0ca59e26a0f3"

            Component.onCompleted: {
                console.log("[QML] VideoPlayer component completed")
                console.log("[QML] VideoPlayer source:", source)
                console.log("[QML] VideoPlayer size:", width, "x", height)
                console.log("[QML] VideoPlayer isPlaying:", isPlaying)
                videoPlayer.forceActiveFocus()
                // Auto-play for testing - delay to allow media to prepare
                playTimer.start()
            }
            
            Timer {
                id: playTimer
                interval: 500 // Wait 500ms for media to prepare
                onTriggered: {
                    console.log("[QML] Auto-playing video")
                    videoPlayer.play()
                }
            }
            
            onSourceChanged: {
                console.log("[QML] VideoPlayer source changed to:", source)
            }
            
            onIsPlayingChanged: {
                console.log("[QML] VideoPlayer isPlaying changed to:", isPlaying)
            }
            
            // Play/Pause button overlay in the center
            Item {
                id: controlsOverlay
                anchors.fill: parent
                visible: true
                
                // Semi-transparent background when paused
                Rectangle {
                    anchors.fill: parent
                    color: videoPlayer.isPlaying ? "transparent" : "#40000000"
                    Behavior on color {
                        ColorAnimation { duration: 200 }
                    }
                }
                
                // Play/Pause button in the center
                Rectangle {
                    id: playPauseButton
                    width: 80
                    height: 80
                    anchors.centerIn: parent
                    radius: width / 2
                    color: "#80000000"
                    border.color: "#FFFFFF"
                    border.width: 2
                    opacity: videoPlayer.isPlaying ? 0 : 1
                    
                    Behavior on opacity {
                        NumberAnimation { duration: 300 }
                    }
                    
                    // Play icon (▶) - shows when paused
                    Text {
                        id: playIcon
                        anchors.centerIn: parent
                        text: "▶"
                        font.pixelSize: 40
                        color: "#FFFFFF"
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
                
                // Click anywhere to toggle play/pause
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
        }
    }
}

