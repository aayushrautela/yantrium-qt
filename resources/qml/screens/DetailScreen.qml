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
            console.log("[DetailScreen] Item details loaded, castFull type:", typeof details.castFull)
            console.log("[DetailScreen] castFull is array:", Array.isArray(details.castFull))
            console.log("[DetailScreen] castFull keys:", Object.keys(details.castFull || {}))
            console.log("[DetailScreen] castFull value:", JSON.stringify(details.castFull))

            // Update cast list model
            root.onCastDataLoaded(details.castFull || [])
            // Load similar items
            if (details.tmdbId) {
                var type = details.type || "movie"
                var tmdbId = parseInt(details.tmdbId) || details.tmdbId
                libraryService.loadSimilarItems(tmdbId, type)
            }
        }
        function onSimilarItemsLoaded(items) {
            root.similarItems = items
            console.log("[DetailScreen] Similar items loaded:", items.length)
        }
        function onError(message) {
            console.error("[DetailScreen] Error:", message)
            root.isLoading = false
        }
    }
    
    property bool isLoading: false
    property int currentTabIndex: 0
    property int selectedSeason: 1
    property var similarItems: []
    
    ListModel {
        id: castModel
    }
    
    function onCastDataLoaded(cast) {
        castModel.clear()
        if (!cast) {
            console.log("[DetailScreen] No cast data provided")
            return
        }
        
        // Convert object with numeric keys to array if needed
        var castArray = []
        if (Array.isArray(cast)) {
            castArray = cast
        } else if (typeof cast === "object") {
            // Convert object with numeric keys to array
            var keys = Object.keys(cast).sort(function(a, b) { return parseInt(a) - parseInt(b) })
            for (var i = 0; i < keys.length; i++) {
                var key = keys[i]
                // Only add if key is numeric (array index)
                if (!isNaN(parseInt(key))) {
                    castArray.push(cast[key])
                }
            }
        } else {
            console.log("[DetailScreen] Cast data is not an array or object:", typeof cast)
            return
        }
        
        console.log("[DetailScreen] Processing cast data, length:", castArray.length)
        // Limit to first 15 items
        var limit = Math.min(castArray.length, 15)
        for (let i = 0; i < limit; i++) {
            let castMember = castArray[i]
            if (!castMember) continue
            
            var profileImageUrl = castMember.profileImageUrl || ""
            var actorName = castMember.name || ""
            var characterName = castMember.character || ""
            
            console.log("[DetailScreen] Adding cast member", i, ":", actorName, "as", characterName, "image:", profileImageUrl ? "yes" : "no")
            
            castModel.append({
                profileImageUrl: profileImageUrl,
                actorName: actorName,
                characterName: characterName
            })
        }
        console.log("[DetailScreen] Cast model populated with", castModel.count, "items")
    }
    
    function getCastList() {
        var cast = root.itemData.castFull
        if (!cast || !Array.isArray(cast)) {
            return []
        }
        return cast
    }
    
    function getTabList() {
        var tabs = ["CAST & CREW", "MORE LIKE THIS", "DETAILS"]
        // Add EPISODES at the front if it's a series
        if (root.itemData.type === "series" || root.itemData.type === "tv") {
            tabs.unshift("EPISODES")
        }
        return tabs
    }
    
    function isSeries() {
        return root.itemData.type === "series" || root.itemData.type === "tv"
    }
    
    function getStackLayoutIndex(tabIndex) {
        // Map tab index to StackLayout index
        // For series: EPISODES(0), CAST(1), MORE(2), DETAILS(3) -> StackLayout: EPISODES(0), CAST(1), MORE(2), DETAILS(3)
        // For movies: CAST(0), MORE(1), DETAILS(2) -> StackLayout: EPISODES(0-hidden), CAST(1), MORE(2), DETAILS(3)
        if (root.isSeries()) {
            return tabIndex
        } else {
            return tabIndex + 1  // Skip EPISODES (index 0)
        }
    }
    
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
                        
                        // Logo or Title (Red text)
                        Item {
                            width: Math.min(parent.width, 600)
                            height: logoImage.visible ? 120 : 80
                            
                            Image {
                                id: logoImage
                                anchors.left: parent.left
                                width: Math.min(parent.width, 600)
                                height: 120
                                fillMode: Image.PreserveAspectFit
                                horizontalAlignment: Image.AlignLeft
                                verticalAlignment: Image.AlignVCenter
                                source: root.itemData.logoUrl || root.itemData.logo || ""
                                visible: (root.itemData.logoUrl || root.itemData.logo) !== "" && logoImage.status === Image.Ready
                                asynchronous: true
                            }
                            
                            Text {
                                anchors.left: parent.left
                                anchors.verticalCenter: parent.verticalCenter
                                text: root.itemData.title || root.itemData.name || ""
                                font.pixelSize: 64
                                font.bold: true
                                color: "#ff0000"
                                visible: !logoImage.visible || logoImage.status === Image.Error
                            }
                        }
                        
                        // Row 1: Release Date + Season Count
                        Row {
                            spacing: 12
                            
                            Text {
                                text: root.itemData.releaseDate || root.itemData.firstAirDate || ""
                                color: "#ffffff"
                                font.pixelSize: 14
                                visible: text !== ""
                            }
                            
                            Text {
                                text: root.itemData.numberOfSeasons > 0 ? root.itemData.numberOfSeasons + " Seasons" : ""
                                color: "#ffffff"
                                font.pixelSize: 14
                                visible: root.itemData.type === "series" && text !== ""
                            }
                        }
                        
                        // Row 2: Content Rating + Genres
                        Row {
                            spacing: 12
                            
                            Text {
                                text: root.itemData.contentRating || ""
                                color: "#ffffff"
                                font.pixelSize: 14
                                visible: text !== ""
                            }
                            
                            Repeater {
                                model: root.itemData.genres || []
                                Text {
                                    text: modelData + (index < (root.itemData.genres || []).length - 1 ? ", " : "")
                                    color: "#ffffff"
                                    font.pixelSize: 14
                                    visible: modelData !== ""
                                }
                            }
                        }
                        
                        // Row 3: Rating Scores
                        Row {
                            spacing: 20
                            
                            // TMDB Score
                            Row {
                                spacing: 4
                                visible: !!(root.itemData.tmdbRating || root.itemData.imdbRating)
                                
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
                            text: root.itemData.description || root.itemData.overview || ""
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
                                text: "+ My List"
                                background: Rectangle {
                                    color: "#40000000"
                                    border.width: 1
                                    border.color: "#ffffff"
                                    radius: 4
                                }
                                contentItem: Row {
                                    spacing: 4
                                    anchors.centerIn: parent
                                    
                                    Text {
                                        text: "+"
                                        font.pixelSize: 18
                                        color: "#ffffff"
                                    }
                                    Text {
                                        text: "My List"
                                        font.pixelSize: 16
                                        color: "#ffffff"
                                    }
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
                                id: tabRepeater
                                model: root.getTabList()
                                
                                Item {
                                    height: 40
                                    width: tabText.width + 20
                                    
                                    Text {
                                        id: tabText
                                        anchors.left: parent.left
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: modelData
                                        font.pixelSize: 18
                                        font.bold: root.currentTabIndex === index
                                        color: "#ffffff"
                                    }
                                    
                                    // Red underline for active tab
                                    Rectangle {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.bottom: parent.bottom
                                        anchors.bottomMargin: 4
                                        height: 2
                                        color: "#ff0000"
                                        visible: root.currentTabIndex === index
                                    }
                                    
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: root.currentTabIndex = index
                                    }
                                }
                            }
                        }
                    }
                    
                    // Tab Content
                    StackLayout {
                        id: tabListView
                        width: parent.width
                        height: 1000  // Set a reasonable height for content
                        currentIndex: root.getStackLayoutIndex(root.currentTabIndex)
                        
                        // EPISODES Tab (index 0, only visible for series)
                        Rectangle {
                            visible: root.isSeries()
                            color: "#09090b"
                            
                            Row {
                                anchors.fill: parent
                                anchors.margins: 60
                                spacing: 40
                                
                                // Left: Season Selector
                                Column {
                                    width: 200
                                    height: parent.height
                                    
                                    Repeater {
                                        model: root.itemData.numberOfSeasons || 0
                                        
                                        Rectangle {
                                            width: parent.width
                                            height: 50
                                            color: root.selectedSeason === (index + 1) ? "#ff0000" : "transparent"
                                            radius: 4
                                            
                                            Text {
                                                anchors.left: parent.left
                                                anchors.leftMargin: 12
                                                anchors.verticalCenter: parent.verticalCenter
                                                text: "Season " + (index + 1)
                                                font.pixelSize: 14
                                                font.bold: root.selectedSeason === (index + 1)
                                                color: root.selectedSeason === (index + 1) ? "#000000" : "#ffffff"
                                            }
                                            
                                            MouseArea {
                                                anchors.fill: parent
                                                onClicked: root.selectedSeason = index + 1
                                            }
                                        }
                                    }
                                    
                                    // Placeholder if no seasons
                                    Item {
                                        width: parent.width
                                        height: 200
                                        visible: (root.itemData.numberOfSeasons || 0) === 0
                                        
                                        Text {
                                            anchors.centerIn: parent
                                            text: "No seasons available"
                                            color: "#666666"
                                            font.pixelSize: 14
                                        }
                                    }
                                }
                                
                                // Right: Episode List Area
                                Column {
                                    width: parent.width - parent.spacing - 200
                                    height: parent.height
                                    spacing: 20
                                    
                                    // Season Header
                                    Text {
                                        text: "Season " + root.selectedSeason + " " + (root.itemData.numberOfSeasons || 0) + " Episodes"
                                        color: "#ffffff"
                                        font.pixelSize: 16
                                        font.bold: true
                                    }
                                    
                                    // Episode List (placeholder structure)
                                    ScrollView {
                                        width: parent.width
                                        height: parent.height - 40
                                        
                                        Column {
                                            width: parent.width
                                            spacing: 20
                                            
                                            // Placeholder episode cards
                                            Repeater {
                                                model: 8  // Placeholder count
                                                
                                                Rectangle {
                                                    width: parent.width
                                                    height: 200
                                                    color: "#1a1a1a"
                                                    radius: 8
                                                    
                                                    Row {
                                                        anchors.fill: parent
                                                        anchors.margins: 16
                                                        spacing: 16
                                                        
                                                        // Episode Thumbnail Placeholder
                                                        Rectangle {
                                                            width: 320
                                                            height: parent.height
                                                            color: "#2a2a2a"
                                                            radius: 4
                                                            
                                                            Text {
                                                                anchors.centerIn: parent
                                                                text: "Episode " + (index + 1)
                                                                color: "#666666"
                                                                font.pixelSize: 14
                                                            }
                                                        }
                                                        
                                                        // Episode Info
                                                        Column {
                                                            width: parent.width - 320 - parent.spacing
                                                            height: parent.height
                                                            spacing: 8
                                                            
                                                            // Badges Row
                                                            Row {
                                                                spacing: 8
                                                                
                                                                // Duration Badge
                                                                Rectangle {
                                                                    width: 50
                                                                    height: 24
                                                                    color: "#ff0000"
                                                                    radius: 4
                                                                    
                                                                    Text {
                                                        anchors.centerIn: parent
                                                                        text: "48m"
                                                                        color: "#ffffff"
                                                                        font.pixelSize: 12
                                                                        font.bold: true
                                                                    }
                                                                }
                                                                
                                                                // Release Date Badge
                                                                Rectangle {
                                                                    width: 100
                                                                    height: 24
                                                                    color: "#ffffff"
                                                                    radius: 4
                                                                    
                                                                    Text {
                                                        anchors.centerIn: parent
                                                                        text: "15-07-2016"
                                                                        color: "#000000"
                                                                        font.pixelSize: 12
                                                                        font.bold: true
                                                                    }
                                                                }
                                                            }
                                                            
                                                            // Episode Number and Title
                                                            Text {
                                                                text: (index + 1) + ". Episode Title Placeholder"
                                                                color: "#ffffff"
                                                                font.pixelSize: 16
                                                                font.bold: true
                                                            }
                                                            
                                                            // Episode Description
                                                            Text {
                                                                width: parent.width
                                                                text: "Episode description placeholder text. This will show the episode synopsis when episode data is available."
                                                                color: "#cccccc"
                                                                font.pixelSize: 14
                                                                wrapMode: Text.WordWrap
                                                                maximumLineCount: 3
                                                                elide: Text.ElideRight
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                            
                                            // Placeholder message
                                            Text {
                                                anchors.horizontalCenter: parent.horizontalCenter
                                                text: "Episodes coming soon"
                                                color: "#666666"
                                                font.pixelSize: 16
                                                visible: (root.itemData.numberOfSeasons || 0) === 0
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        
                        // CAST & CREW Tab (index 0 for movies, index 1 for series)
                        Rectangle {
                            color: "#09090b"
                            visible: true
                            
                            // Inner ListView container
                            Item {
                                width: parent.width - 100  // Reduce width to leave 50px on each side
                                height: 420
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.top: parent.top
                                anchors.topMargin: 20  // Reduced spacing from tab bar
                                
                                ListView {
                                    id: castListView
                                    anchors.fill: parent
                                    orientation: ListView.Horizontal
                                    spacing: 24
                                    leftMargin: 0
                                    rightMargin: 0
                                    clip: true
                                    model: castModel
                                    
                                    PropertyAnimation {
                                        id: castScrollAnimation
                                        target: castListView
                                        property: "contentX"
                                        duration: 300
                                        easing.type: Easing.OutCubic
                                    }
                                    
                                    delegate: Loader {
                                        width: 240
                                        height: 360
                                        source: "qrc:/qml/components/CastCard.qml"
                                        
                                        onLoaded: {
                                            if (item) {
                                                item.profileImageUrl = model.profileImageUrl || ""
                                                item.actorName = model.actorName || ""
                                                item.characterName = model.characterName || ""
                                            }
                                        }
                                    }
                                }
                                
                                // Left arrow
                                Item {
                                    anchors.left: parent.left
                                    anchors.verticalCenter: castListView.verticalCenter
                                    anchors.verticalCenterOffset: -78  // Shift up to center on image (image center at 132px, container center at 210px)
                                    width: 48
                                    height: 48
                                    visible: castListView.contentX > 0
                                    
                                    property bool isHovered: false
                                    
                                    Image {
                                        anchors.centerIn: parent
                                        width: 48
                                        height: 48
                                        source: "qrc:/assets/icons/arrow-left.svg"
                                        fillMode: Image.PreserveAspectFit
                                        opacity: parent.isHovered ? 1.0 : 0.4
                                        Behavior on opacity { NumberAnimation { duration: 200 } }
                                    }
                                    
                                    MouseArea {
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onEntered: parent.isHovered = true
                                        onExited: parent.isHovered = false
                                        onClicked: {
                                            var targetX = Math.max(0, castListView.contentX - castListView.width * 0.8)
                                            castScrollAnimation.to = targetX
                                            castScrollAnimation.start()
                                        }
                                    }
                                }
                                
                                // Right arrow
                                Item {
                                    anchors.right: parent.right
                                    anchors.verticalCenter: castListView.verticalCenter
                                    anchors.verticalCenterOffset: -78  // Shift up to center on image (image center at 132px, container center at 210px)
                                    width: 48
                                    height: 48
                                    visible: castListView.contentX < castListView.contentWidth - castListView.width
                                    
                                    property bool isHovered: false
                                    
                                    Image {
                                        anchors.centerIn: parent
                                        width: 48
                                        height: 48
                                        source: "qrc:/assets/icons/arrow-right.svg"
                                        fillMode: Image.PreserveAspectFit
                                        opacity: parent.isHovered ? 1.0 : 0.4
                                        Behavior on opacity { NumberAnimation { duration: 200 } }
                                    }
                                    
                                    MouseArea {
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onEntered: parent.isHovered = true
                                        onExited: parent.isHovered = false
                                        onClicked: {
                                            var targetX = Math.min(
                                                castListView.contentWidth - castListView.width,
                                                castListView.contentX + castListView.width * 0.8
                                            )
                                            castScrollAnimation.to = targetX
                                            castScrollAnimation.start()
                                        }
                                    }
                                }
                                
                                // Empty state
                                Text {
                                    anchors.centerIn: parent
                                    text: "No cast information available"
                                    color: "#666666"
                                    font.pixelSize: 16
                                    visible: castModel.count === 0
                                }
                            }
                        }
                        
                        // MORE LIKE THIS Tab
                        Rectangle {
                            color: "#09090b"
                            visible: true
                            
                            // Inner ListView container
                            Item {
                                width: parent.width - 100  // Reduce width to leave 50px on each side
                                height: 420
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.top: parent.top
                                anchors.topMargin: 20  // Reduced spacing from tab bar
                                
                                ListView {
                                    id: similarListView
                                    anchors.fill: parent
                                    orientation: ListView.Horizontal
                                    spacing: 24
                                    leftMargin: 0
                                    rightMargin: 0
                                    clip: true
                                    model: root.similarItems || []
                                    
                                    PropertyAnimation {
                                        id: similarScrollAnimation
                                        target: similarListView
                                        property: "contentX"
                                        duration: 300
                                        easing.type: Easing.OutCubic
                                    }
                                    
                                    delegate: Item {
                                        width: 240
                                        height: 400
                                        
                                        property bool isHovered: false
                                        property var itemData: modelData
                                        
                                        MouseArea {
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            z: 100  // Ensure MouseArea is on top
                                            onEntered: parent.isHovered = true
                                            onExited: parent.isHovered = false
                                            onClicked: {
                                                var similarItem = parent.itemData
                                                console.log("[DetailScreen] Similar item clicked, modelData:", similarItem)
                                                console.log("[DetailScreen] Item ID:", similarItem ? similarItem.id : "undefined")
                                                console.log("[DetailScreen] Item type:", similarItem ? similarItem.type : "undefined")
                                                
                                                if (similarItem) {
                                                    var contentId = similarItem.id || ""
                                                    var itemType = similarItem.type || "movie"
                                                    console.log("[DetailScreen] Loading details for:", contentId, itemType)
                                                    if (contentId) {
                                                        root.loadDetails(contentId, itemType, "")
                                                    } else {
                                                        console.error("[DetailScreen] No contentId in similar item")
                                                    }
                                                } else {
                                                    console.error("[DetailScreen] Similar item is null or undefined")
                                                }
                                            }
                                        }
                                        
                                        Column {
                                            anchors.fill: parent
                                            spacing: 12
                                            
                                            // Poster
                                            Item {
                                                id: imageContainer
                                                width: parent.width
                                                height: 360
                                                
                                                Rectangle { 
                                                    id: maskShape
                                                    anchors.fill: parent
                                                    radius: 8
                                                    visible: false
                                                }
                                                layer.enabled: true
                                                layer.effect: OpacityMask { 
                                                    maskSource: maskShape
                                                }
                                                
                                                Rectangle { 
                                                    anchors.fill: parent
                                                    color: "#2a2a2a"
                                                    radius: 8
                                                }
                                                
                                                Image {
                                                    id: img
                                                    anchors.fill: parent
                                                    source: itemData.posterUrl || ""
                                                    fillMode: Image.PreserveAspectCrop
                                                    asynchronous: true
                                                    smooth: true
                                                    scale: isHovered ? 1.10 : 1.0
                                                    Behavior on scale { 
                                                        NumberAnimation { duration: 300; easing.type: Easing.OutQuad }
                                                    }
                                                }
                                                
                                                // Rating
                                                Rectangle {
                                                    anchors.bottom: parent.bottom
                                                    anchors.right: parent.right
                                                    anchors.margins: 8
                                                    visible: itemData.rating !== ""
                                                    color: "black"
                                                    opacity: 0.8
                                                    radius: 4
                                                    width: 40
                                                    height: 20
                                                    
                                                    Row {
                                                        anchors.centerIn: parent
                                                        spacing: 2
                                                        
                                                        Text { 
                                                            text: "\u2605"
                                                            color: "#ffffff"
                                                            font.pixelSize: 10
                                                        }
                                                        Text { 
                                                            text: itemData.rating || ""
                                                            color: "white"
                                                            font.pixelSize: 10
                                                            font.bold: true
                                                        }
                                                    }
                                                }
                                                
                                                // Border
                                                Rectangle {
                                                    anchors.fill: parent
                                                    color: "transparent"
                                                    radius: 8
                                                    z: 1
                                                    border.width: 3
                                                    border.color: isHovered ? "#ffffff" : "transparent"
                                                    Behavior on border.color { 
                                                        ColorAnimation { duration: 200 }
                                                    }
                                                }
                                            }
                                            
                                            // Title
                                            Text {
                                                width: parent.width
                                                text: itemData.title || itemData.name || ""
                                                color: "white"
                                                font.pixelSize: 14
                                                font.bold: true
                                                elide: Text.ElideRight
                                                horizontalAlignment: Text.AlignHCenter
                                            }
                                        }
                                    }
                                }
                                
                                // Left arrow
                                Item {
                                    anchors.left: parent.left
                                    anchors.verticalCenter: similarListView.verticalCenter
                                    anchors.verticalCenterOffset: -20  // Shift up by half the text area (40px / 2)
                                    width: 48
                                    height: 48
                                    visible: similarListView.contentX > 0
                                    
                                    property bool isHovered: false
                                    
                                    Image {
                                        anchors.centerIn: parent
                                        width: 48
                                        height: 48
                                        source: "qrc:/assets/icons/arrow-left.svg"
                                        fillMode: Image.PreserveAspectFit
                                        opacity: parent.isHovered ? 1.0 : 0.4
                                        Behavior on opacity { NumberAnimation { duration: 200 } }
                                    }
                                    
                                    MouseArea {
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onEntered: parent.isHovered = true
                                        onExited: parent.isHovered = false
                                        onClicked: {
                                            var targetX = Math.max(0, similarListView.contentX - similarListView.width * 0.8)
                                            similarScrollAnimation.to = targetX
                                            similarScrollAnimation.start()
                                        }
                                    }
                                }
                                
                                // Right arrow
                                Item {
                                    anchors.right: parent.right
                                    anchors.verticalCenter: similarListView.verticalCenter
                                    anchors.verticalCenterOffset: -20  // Shift up by half the text area (40px / 2)
                                    width: 48
                                    height: 48
                                    visible: similarListView.contentX < similarListView.contentWidth - similarListView.width
                                    
                                    property bool isHovered: false
                                    
                                    Image {
                                        anchors.centerIn: parent
                                        width: 48
                                        height: 48
                                        source: "qrc:/assets/icons/arrow-right.svg"
                                        fillMode: Image.PreserveAspectFit
                                        opacity: parent.isHovered ? 1.0 : 0.4
                                        Behavior on opacity { NumberAnimation { duration: 200 } }
                                    }
                                    
                                    MouseArea {
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onEntered: parent.isHovered = true
                                        onExited: parent.isHovered = false
                                        onClicked: {
                                            var targetX = Math.min(
                                                similarListView.contentWidth - similarListView.width,
                                                similarListView.contentX + similarListView.width * 0.8
                                            )
                                            similarScrollAnimation.to = targetX
                                            similarScrollAnimation.start()
                                        }
                                    }
                                }
                                
                                // Empty state
                                Text {
                                    anchors.centerIn: parent
                                    text: "No similar items available"
                                    color: "#666666"
                                    font.pixelSize: 16
                                    visible: (root.similarItems || []).length === 0 && !root.isLoading
                                }
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
