import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import Yantrium.Services 1.0

Item {
    id: root
    
    property var itemData: ({})  // QVariantMap from parent
    property LibraryService libraryService: LibraryService {
        id: libraryService
    }
    
    Connections {
        target: libraryService
        function onItemDetailsLoaded(details) {
            root.itemData = details
            root.isLoading = false
        }
        function onError(message) {
            console.error("[DetailScreen] Error:", message)
            root.isLoading = false
        }
    }
    
    property bool isLoading: false
    
    signal closeRequested()
    
    function loadDetails(contentId, type, addonId) {
        if (!contentId || !type) {
            console.error("[DetailScreen] Missing contentId or type")
            return
        }
        isLoading = true
        libraryService.loadItemDetails(contentId, type, addonId || "")
    }
    
    Rectangle {
        anchors.fill: parent
        color: "#09090b"
        
        ScrollView {
            anchors.fill: parent
            contentWidth: width
            contentHeight: contentColumn.height
            clip: true
            
            Column {
                id: contentColumn
                width: parent.width
                spacing: 0
                
                // Backdrop Section
                Item {
                    width: parent.width
                    height: 600
                    
                    // Backdrop Image
                    Image {
                        id: backdropImage
                        anchors.fill: parent
                        source: root.itemData.backdropUrl || root.itemData.background || ""
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                    }
                    
                    // Gradient Overlay
                    Rectangle {
                        anchors.fill: parent
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#00000000" }
                            GradientStop { position: 0.5; color: "#8009090b" }
                            GradientStop { position: 1.0; color: "#ff09090b" }
                        }
                    }
                    
                    // Back Button
                    Item {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.margins: 20
                        width: 48
                        height: 48
                        z: 10
                        
                        property bool isHovered: false
                        
                        Image {
                            anchors.centerIn: parent
                            width: 24
                            height: 24
                            source: "qrc:/assets/icons/arrow-left.svg"
                            fillMode: Image.PreserveAspectFit
                            opacity: parent.isHovered ? 1.0 : 0.7
                            Behavior on opacity { NumberAnimation { duration: 200 } }
                        }
                        
                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: parent.isHovered = true
                            onExited: parent.isHovered = false
                            onClicked: root.closeRequested()
                        }
                    }
                    
                    // Title and Info Section (overlay on backdrop)
                    Column {
                        anchors.left: parent.left
                        anchors.bottom: parent.bottom
                        anchors.right: parent.right
                        anchors.leftMargin: 60
                        anchors.rightMargin: 40
                        anchors.bottomMargin: 40
                        spacing: 16
                        
                        // Logo or Title
                        Item {
                            width: Math.min(parent.width, 600)
                            height: logoImage.visible ? 120 : 80
                            
                            Image {
                                id: logoImage
                                anchors.left: parent.left
                                width: Math.min(parent.width, 600)
                                height: 120
                                fillMode: Image.PreserveAspectFit
                                source: root.itemData.logoUrl || root.itemData.logo || ""
                                visible: (root.itemData.logoUrl || root.itemData.logo) !== ""
                                asynchronous: true
                            }
                            
                            Text {
                                anchors.left: parent.left
                                anchors.verticalCenter: parent.verticalCenter
                                text: root.itemData.title || root.itemData.name || ""
                                font.pixelSize: 56
                                font.bold: true
                                color: "#ffffff"
                                visible: !logoImage.visible || logoImage.status === Image.Error
                            }
                        }
                        
                        // Metadata Row
                        Row {
                            spacing: 12
                            
                            // Release Date
                            Text {
                                text: root.itemData.releaseDate || root.itemData.releaseInfo || ""
                                color: "#ffffff"
                                font.pixelSize: 14
                                visible: text !== ""
                            }
                            
                            // Content Rating
                            Text {
                                text: root.itemData.contentRating || ""
                                color: "#ffffff"
                                font.pixelSize: 14
                                visible: text !== ""
                            }
                            
                            // Genres
                            Repeater {
                                model: root.itemData.genres || []
                                Text {
                                    text: modelData
                                    color: "#ffffff"
                                    font.pixelSize: 14
                                    visible: modelData !== ""
                                }
                            }
                        }
                        
                        // Scores Row
                        Row {
                            spacing: 20
                            
                            // TMDB Score
                            Row {
                                spacing: 4
                                visible: root.itemData.tmdbRating || root.itemData.imdbRating
                                
                                Text {
                                    text: "TMDB:"
                                    color: "#aaaaaa"
                                    font.pixelSize: 14
                                }
                                Text {
                                    text: root.itemData.tmdbRating || root.itemData.imdbRating || ""
                                    color: "#ffffff"
                                    font.pixelSize: 14
                                    font.bold: true
                                }
                            }
                            
                            // IMDb Score
                            Row {
                                spacing: 4
                                visible: root.itemData.imdbRating !== ""
                                
                                Text {
                                    text: "IMDb:"
                                    color: "#aaaaaa"
                                    font.pixelSize: 14
                                }
                                Text {
                                    text: root.itemData.imdbRating || ""
                                    color: "#ffffff"
                                    font.pixelSize: 14
                                    font.bold: true
                                }
                            }
                            
                            // RT Score (placeholder)
                            Row {
                                spacing: 4
                                visible: root.itemData.rtRating !== ""
                                
                                Text {
                                    text: "RT:"
                                    color: "#aaaaaa"
                                    font.pixelSize: 14
                                }
                                Text {
                                    text: root.itemData.rtRating || ""
                                    color: "#ffffff"
                                    font.pixelSize: 14
                                    font.bold: true
                                }
                            }
                            
                            // MC Score (placeholder)
                            Row {
                                spacing: 4
                                visible: root.itemData.mcRating !== ""
                                
                                Text {
                                    text: "MC:"
                                    color: "#aaaaaa"
                                    font.pixelSize: 14
                                }
                                Text {
                                    text: root.itemData.mcRating || ""
                                    color: "#ffffff"
                                    font.pixelSize: 14
                                    font.bold: true
                                }
                            }
                        }
                        
                        // Description
                        Text {
                            width: Math.min(parent.width * 0.75, 800)
                            text: root.itemData.description || ""
                            color: "#e0e0e0"
                            font.pixelSize: 16
                            wrapMode: Text.WordWrap
                            lineHeight: 1.5
                            visible: text !== ""
                        }
                        
                        // Action Buttons
                        Row {
                            spacing: 12
                            
                            Button {
                                width: 140
                                height: 44
                                text: "Play"
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
                                onClicked: {
                                    console.log("Play clicked for:", root.itemData.title || root.itemData.name)
                                    // TODO: Implement playback
                                }
                            }
                            
                            Button {
                                width: 140
                                height: 44
                                text: "My List"
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
                                onClicked: {
                                    console.log("My List clicked for:", root.itemData.title || root.itemData.name)
                                    // TODO: Implement add to list
                                }
                            }
                        }
                    }
                }
                
                // Tabs Section
                Column {
                    width: parent.width
                    spacing: 0
                    
                    // Tab Bar
                    Rectangle {
                        width: parent.width
                        height: 60
                        color: "#09090b"
                        border.width: 0
                        border.color: "#333333"
                        
                        Row {
                            anchors.left: parent.left
                            anchors.leftMargin: 60
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 40
                            
                            Repeater {
                                model: ["CAST & CREW", "MORE LIKE THIS", "DETAILS"]
                                
                                Rectangle {
                                    height: 40
                                    color: tabListView.currentIndex === index ? "#bb86fc" : "transparent"
                                    radius: 4
                                    
                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData
                                        font.pixelSize: 14
                                        font.bold: tabListView.currentIndex === index
                                        color: tabListView.currentIndex === index ? "#000000" : "#ffffff"
                                    }
                                    
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: tabListView.currentIndex = index
                                    }
                                }
                            }
                        }
                    }
                    
                    // Tab Content
                    StackLayout {
                        id: tabListView
                        width: parent.width
                        height: 400
                        currentIndex: 0
                        
                        // CAST & CREW Tab (placeholder)
                        Rectangle {
                            color: "#09090b"
                            Text {
                                anchors.centerIn: parent
                                text: "CAST & CREW\n(Coming Soon)"
                                color: "#666666"
                                font.pixelSize: 16
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }
                        
                        // MORE LIKE THIS Tab (placeholder)
                        Rectangle {
                            color: "#09090b"
                            Text {
                                anchors.centerIn: parent
                                text: "MORE LIKE THIS\n(Coming Soon)"
                                color: "#666666"
                                font.pixelSize: 16
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }
                        
                        // DETAILS Tab (placeholder)
                        Rectangle {
                            color: "#09090b"
                            Text {
                                anchors.centerIn: parent
                                text: "DETAILS\n(Coming Soon)"
                                color: "#666666"
                                font.pixelSize: 16
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }
                    }
                }
                
                // Loading State
                Rectangle {
                    width: parent.width
                    height: 400
                    color: "#09090b"
                    visible: root.isLoading
                    
                    Text {
                        anchors.centerIn: parent
                        text: "Loading..."
                        color: "#666666"
                        font.pixelSize: 16
                    }
                }
            }
        }
    }
}

