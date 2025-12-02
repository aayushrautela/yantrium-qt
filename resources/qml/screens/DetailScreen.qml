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
    
    // TraktAuthService is a singleton, accessed directly
    property TraktAuthService traktAuthService: TraktAuthService
    
    property TraktWatchlistService traktWatchlist: TraktWatchlistService {
        id: traktWatchlistService
    }
    
    property LocalLibraryService localLibrary: LocalLibraryService {
        id: localLibraryService
    }
    
    property bool isInLibrary: false
    
    Connections {
        target: libraryService
        function onItemDetailsLoaded(details) {
            root.itemData = details
            root.isLoading = false
            
            // Update cast list model
            root.onCastDataLoaded(details.castFull || [])
            // Load similar items
            if (details.tmdbId) {
                var type = details.type || "movie"
                var tmdbId = parseInt(details.tmdbId) || details.tmdbId
                libraryService.loadSimilarItems(tmdbId, type)
            }
            
            // Check if item is in library
            checkIfInLibrary()
        }
        function onSimilarItemsLoaded(items) {
            root.similarItems = items
        }
        function onError(message) {
            console.error("[DetailScreen] Error:", message)
            root.isLoading = false
        }
    }
    
    Connections {
        target: traktWatchlistService
        function onWatchlistItemAdded(success) {
            if (success) {
                console.log("[DetailScreen] Successfully added to Trakt watchlist")
                root.isInLibrary = true
            } else {
                console.error("[DetailScreen] Failed to add to Trakt watchlist")
            }
        }
        function onIsInWatchlistResult(inWatchlist) {
            root.isInLibrary = inWatchlist
        }
        function onError(message) {
            console.error("[DetailScreen] Trakt watchlist error:", message)
        }
    }
    
    Connections {
        target: localLibraryService
        function onLibraryItemAdded(success) {
            if (success) {
                console.log("[DetailScreen] Successfully added to local library")
                root.isInLibrary = true
            } else {
                console.error("[DetailScreen] Failed to add to local library")
            }
        }
        function onIsInLibraryResult(inLibrary) {
            root.isInLibrary = inLibrary
        }
        function onError(message) {
            console.error("[DetailScreen] Local library error:", message)
        }
    }
    
    Connections {
        target: traktAuthService
        function onAuthenticationStatusChanged(authenticated) {
            console.log("[DetailScreen] Authentication status changed:", authenticated)
            // Re-check if item is in library when auth status changes
            if (root.itemData && Object.keys(root.itemData).length > 0) {
                checkIfInLibrary()
            }
        }
    }
    
    function checkIfInLibrary() {
        var contentId = root.itemData.imdbId || root.itemData.id || ""
        if (!contentId) return
        
        if (traktAuthService.isAuthenticated) {
            // Check Trakt watchlist
            var type = root.itemData.type || "movie"
            if (type === "tv" || type === "series") {
                type = "show"
            } else {
                type = "movie"
            }
            traktWatchlistService.isInWatchlist(contentId, type)
        } else {
            // Check local library
            localLibraryService.isInLibrary(contentId)
        }
    }
    
    Component.onCompleted: {
        // Check authentication status when component loads
        console.log("[DetailScreen] Component completed, checking authentication")
        traktAuthService.checkAuthentication()
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
        if (!cast) return
        
        var castArray = []
        if (Array.isArray(cast)) {
            castArray = cast
        } else if (typeof cast === "object") {
            var keys = Object.keys(cast).sort(function(a, b) { return parseInt(a) - parseInt(b) })
            for (var i = 0; i < keys.length; i++) {
                var key = keys[i]
                if (!isNaN(parseInt(key))) castArray.push(cast[key])
            }
        }
        
        var limit = Math.min(castArray.length, 15)
        for (let i = 0; i < limit; i++) {
            let castMember = castArray[i]
            if (!castMember) continue
            castModel.append({
                profileImageUrl: castMember.profileImageUrl || "",
                actorName: castMember.name || "",
                characterName: castMember.character || ""
            })
        }
    }
    
    function getCastList() {
        return root.itemData.castFull || []
    }
    
    function getTabList() {
        var tabs = ["CAST & CREW", "MORE LIKE THIS", "DETAILS"]
        if (root.itemData.type === "series" || root.itemData.type === "tv") {
            tabs.unshift("EPISODES")
        }
        return tabs
    }
    
    function isSeries() {
        return root.itemData.type === "series" || root.itemData.type === "tv"
    }
    
    function getStackLayoutIndex(tabIndex) {
        if (root.isSeries()) return tabIndex
        return tabIndex + 1
    }
    
    signal closeRequested()
    
    function loadDetails(contentId, type, addonId) {
        if (!contentId || !type) return
        isLoading = true
        libraryService.loadItemDetails(contentId, type, addonId || "")
    }
    
    Rectangle {
        anchors.fill: parent
        color: "#09090b"
        
        // --- BACKDROP IMAGE FIX ---
        // Position: Top Right
        // Coverage: 85% of screen width
        // Behavior: Corner to Corner (Crop to fill area)
        Image {
            id: backdropImage
            
            // Anchor firmly to the Right and Top of the parent
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.rightMargin: 0
            anchors.topMargin: 0
            
            // Width is exactly 85% of the screen
            width: parent.width * 0.85
            
            // Height is full screen to ensure vertical coverage
            height: parent.height
            
            source: root.itemData.backdropUrl || root.itemData.background || ""
            
            // Use PreserveAspectCrop to ensure the image fills the 70% area completely
            // without leaving empty space (black bars), while keeping aspect ratio.
            fillMode: Image.PreserveAspectCrop
            
            // Align the cropped content to the Top Right to ensure the "corner" is filled
            horizontalAlignment: Image.AlignRight
            verticalAlignment: Image.AlignTop
            
            asynchronous: true
            z: 0 // Keep behind content
        }
        
        // Gradient overlay to fade backdrop edges into background
        // Horizontal gradient: fade from left edge (opaque) to right (transparent)
        Rectangle {
            id: backdropGradientHorizontal
            anchors.right: backdropImage.right
            anchors.top: backdropImage.top
            width: backdropImage.width
            height: backdropImage.height
            z: 1 // Above backdrop image, below content
            
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { 
                    position: 0.0; 
                    color: "#ff09090b" // Fully opaque at left edge
                }
                GradientStop { 
                    position: 0.15; 
                    color: "#0309090b" // Quick fade to ~1% darkening
                }
                GradientStop { 
                    position: 1.0; 
                    color: "#0309090b" // Maintain ~1% darkening for the rest
                }
            }
        }
        
        // Vertical gradient: fade from bottom edge (opaque) to top (transparent)
        Rectangle {
            id: backdropGradientVertical
            anchors.right: backdropImage.right
            anchors.top: backdropImage.top
            width: backdropImage.width
            height: backdropImage.height
            z: 1 // Above backdrop image, below content
            
            gradient: Gradient {
                orientation: Gradient.Vertical
                GradientStop { 
                    position: 0.0; 
                    color: "#ff09090b" // Fully opaque at bottom edge
                }
                GradientStop { 
                    position: 0.3; 
                    color: "#0309090b" // Quick fade to ~1% darkening
                }
                GradientStop { 
                    position: 1.0; 
                    color: "#0309090b" // Maintain ~1% darkening for the rest
                }
            }
        }
        
        ScrollView {
            anchors.fill: parent
            contentWidth: width
            contentHeight: contentColumn.height
            clip: true
            z: 2
            
            Column {
                id: contentColumn
                width: parent.width
                spacing: 0
                
                // Backdrop Section
                Item {
                    width: parent.width
                    height: Math.max(600, root.height * 0.85)
                    
                    // Gradient Overlay - Full screen
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
                            sourceSize.width: 48
                            sourceSize.height: 48
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                            antialiasing: true
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
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.verticalCenterOffset: 30
                        anchors.right: parent.right
                        anchors.leftMargin: 120
                        anchors.rightMargin: 40
                        spacing: 24
                        
                        // Logo or Title (Red text)
                        Item {
                            width: Math.min(parent.width, 600)
                            height: logoImage.visible ? Math.max(logoImage.paintedHeight, 100) : 100
                            
                            Image {
                                id: logoImage
                                anchors.left: parent.left
                                anchors.top: parent.top
                                width: Math.min(parent.width, 600)
                                fillMode: Image.PreserveAspectFit
                                horizontalAlignment: Image.AlignLeft
                                source: root.itemData.logoUrl || root.itemData.logo || ""
                                visible: (root.itemData.logoUrl || root.itemData.logo) !== "" && logoImage.status === Image.Ready
                                asynchronous: true
                            }
                            
                            Text {
                                anchors.left: parent.left
                                anchors.top: parent.top
                                text: root.itemData.title || root.itemData.name || ""
                                font.pixelSize: 80
                                font.bold: true
                                color: "#ff0000"
                                visible: !logoImage.visible || logoImage.status === Image.Error
                            }
                        }
                        
                        // Row 1: Release Date • Content Rating • Genres
                        Row {
                            spacing: 8
                            
                            Text {
                                text: root.itemData.releaseDate || root.itemData.firstAirDate || ""
                                color: "#ffffff"
                                font.pixelSize: 18
                                visible: text !== ""
                            }
                            
                            Text {
                                text: "•"
                                color: "#ffffff"
                                font.pixelSize: 18
                                visible: (root.itemData.releaseDate || root.itemData.firstAirDate) !== "" && 
                                         (root.itemData.contentRating !== "" || (root.itemData.genres || []).length > 0)
                            }
                            
                            Text {
                                text: root.itemData.contentRating || ""
                                color: "#ffffff"
                                font.pixelSize: 18
                                visible: text !== ""
                            }
                            
                            Text {
                                text: "•"
                                color: "#ffffff"
                                font.pixelSize: 18
                                visible: root.itemData.contentRating !== "" && (root.itemData.genres || []).length > 0
                            }
                            
                            Repeater {
                                id: genresRepeater
                                model: {
                                    var genres = root.itemData.genres || []
                                    var limitedGenres = []
                                    var maxGenres = Math.min(3, genres.length)
                                    for (var i = 0; i < maxGenres; i++) {
                                        if (genres[i]) {
                                            limitedGenres.push(genres[i])
                                        }
                                    }
                                    return limitedGenres
                                }
                                Text {
                                    text: modelData + (index < genresRepeater.count - 1 ? " " : "")
                                    color: "#ffffff"
                                    font.pixelSize: 18
                                    visible: modelData !== ""
                                }
                            }
                        }
                        
                        // Row 2: Season Count (for series only)
                        Row {
                            spacing: 12
                            visible: root.itemData.type === "series" && root.itemData.numberOfSeasons > 0
                            
                            Text {
                                text: root.itemData.numberOfSeasons + " Seasons"
                                color: "#ffffff"
                                font.pixelSize: 18
                            }
                        }
                        
                        // Row 3: Rating Scores
                        Row {
                            spacing: 20
                            
                            // TMDB Score
                            Row {
                                spacing: 6
                                visible: !!(root.itemData.tmdbRating || root.itemData.imdbRating)
                                
                                Image {
                                    width: 50
                                    height: 22
                                    source: "qrc:/assets/icons/tmdb.svg"
                                    sourceSize.width: 100
                                    sourceSize.height: 44
                                    fillMode: Image.PreserveAspectFit
                                    anchors.verticalCenter: parent.verticalCenter
                                    smooth: true
                                    antialiasing: true
                                }
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: root.itemData.tmdbRating || root.itemData.imdbRating || ""
                                    color: "#ffffff"
                                    font.pixelSize: 18
                                    font.bold: true
                                }
                            }
                            
                            // IMDb Score
                            Row {
                                spacing: 6
                                visible: {
                                    var rating = root.itemData.imdbRating
                                    return rating !== undefined && rating !== null && rating !== ""
                                }
                                
                                Image {
                                    width: 40
                                    height: 20
                                    source: "qrc:/assets/icons/imdb.svg"
                                    sourceSize.width: 80
                                    sourceSize.height: 40
                                    fillMode: Image.PreserveAspectFit
                                    anchors.verticalCenter: parent.verticalCenter
                                    smooth: true
                                    antialiasing: true
                                }
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: {
                                        var rating = root.itemData.imdbRating
                                        return (rating !== undefined && rating !== null) ? rating : ""
                                    }
                                    color: "#ffffff"
                                    font.pixelSize: 18
                                    font.bold: true
                                }
                            }
                            
                            // Metacritic Score
                            Row {
                                spacing: 6
                                visible: {
                                    var score = root.itemData.metascore
                                    return score !== undefined && score !== null && score !== ""
                                }
                                
                                Image {
                                    width: 20
                                    height: 20
                                    source: "qrc:/assets/icons/metacritic.svg"
                                    sourceSize.width: 40
                                    sourceSize.height: 40
                                    fillMode: Image.PreserveAspectFit
                                    anchors.verticalCenter: parent.verticalCenter
                                    smooth: true
                                    antialiasing: true
                                }
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: {
                                        var score = root.itemData.metascore
                                        return (score !== undefined && score !== null) ? score : ""
                                    }
                                    color: "#ffffff"
                                    font.pixelSize: 18
                                    font.bold: true
                                }
                            }
                            
                            // Rotten Tomatoes Score (from OMDB ratings)
                            Repeater {
                                model: {
                                    try {
                                        var ratings = root.itemData.omdbRatings
                                        if (!ratings || !Array.isArray(ratings)) {
                                            return []
                                        }
                                        var rtRatings = []
                                        for (var i = 0; i < ratings.length; i++) {
                                            if (ratings[i] && ratings[i].source === "Rotten Tomatoes") {
                                                rtRatings.push(ratings[i])
                                            }
                                        }
                                        return rtRatings
                                    } catch (e) {
                                        return []
                                    }
                                }
                                
                                Row {
                                    spacing: 6
                                    
                                    Image {
                                        width: 20
                                        height: 20
                                        source: "qrc:/assets/icons/rotten_tomatoes.svg"
                                        sourceSize.width: 40
                                        sourceSize.height: 40
                                        fillMode: Image.PreserveAspectFit
                                        anchors.verticalCenter: parent.verticalCenter
                                        smooth: true
                                        antialiasing: true
                                    }
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: {
                                            try {
                                                return modelData && modelData.value ? modelData.value : ""
                                            } catch (e) {
                                                return ""
                                            }
                                        }
                                        color: "#ffffff"
                                        font.pixelSize: 18
                                        font.bold: true
                                    }
                                }
                            }
                        }
                        
                        // Description
                        Text {
                            width: Math.min(parent.width * 0.75, 800)
                            text: root.itemData.description || root.itemData.overview || ""
                            color: "#e0e0e0"
                            font.pixelSize: 20
                            wrapMode: Text.WordWrap
                            lineHeight: 1.2
                            visible: text !== ""
                        }
                        
                        // Action Buttons
                        Row {
                            spacing: 12
                            
                            Button {
                                width: 140
                                height: 44
                                background: Rectangle {
                                    color: "#ffffff"
                                    radius: 22  // Pill shape (half of height)
                                }
                                contentItem: Item {
                                    Row {
                                        spacing: 8
                                        anchors.centerIn: parent
                                        
                                        Image {
                                            width: 20
                                            height: 20
                                            source: "qrc:/assets/icons/play_button.svg"
                                            sourceSize.width: 40
                                            sourceSize.height: 40
                                            fillMode: Image.PreserveAspectFit
                                            anchors.verticalCenter: parent.verticalCenter
                                            smooth: true
                                            antialiasing: true
                                        }
                                        Text {
                                            text: "Play"
                                            font.pixelSize: 18
                                    font.bold: true
                                            color: "#000000"
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                    }
                                }
                                onClicked: {
                                    console.log("Play clicked for:", root.itemData.title || root.itemData.name)
                                }
                            }
                            
                            Button {
                                width: 140
                                height: 44
                                background: Rectangle {
                                    color: "#40000000"
                                    border.width: 1
                                    border.color: "#ffffff"
                                    radius: 4
                                }
                                contentItem: Item {
                                    Row {
                                    spacing: 4
                                    anchors.centerIn: parent
                                    
                                    Text {
                                        text: root.isInLibrary ? "✓" : "+"
                                        font.pixelSize: 20
                                        color: "#ffffff"
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    Text {
                                        text: "Library"
                                        font.pixelSize: 18
                                        color: "#ffffff"
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    }
                                }
                                onClicked: {
                                    var contentId = root.itemData.imdbId || root.itemData.id || ""
                                    var type = root.itemData.type || "movie"
                                    
                                    if (!contentId) {
                                        console.error("[DetailScreen] Cannot add to library: no contentId")
                                        return
                                    }
                                    
                                    // Normalize type
                                    if (type === "tv" || type === "series") {
                                        type = "show"
                                    } else {
                                        type = "movie"
                                    }
                                    
                                    // Debug: Check authentication status
                                    console.log("[DetailScreen] Trakt auth status:", traktAuthService.isAuthenticated)
                                    console.log("[DetailScreen] Trakt configured:", traktAuthService.isConfigured)
                                    
                                    if (traktAuthService.isAuthenticated) {
                                        // Add to Trakt watchlist
                                        console.log("[DetailScreen] Adding to Trakt watchlist:", contentId, type)
                                        traktWatchlistService.addToWatchlist(type, contentId)
                                    } else {
                                        // Add to local library
                                        console.log("[DetailScreen] Adding to local library:", contentId, type)
                                        
                                        var libraryItem = {
                                            contentId: contentId,
                                            type: type === "show" ? "series" : "movie",
                                            title: root.itemData.title || root.itemData.name || "",
                                            year: root.itemData.year || 0,
                                            posterUrl: root.itemData.posterUrl || "",
                                            backdropUrl: root.itemData.backdropUrl || "",
                                            logoUrl: root.itemData.logoUrl || "",
                                            description: root.itemData.description || root.itemData.overview || "",
                                            rating: root.itemData.rating || root.itemData.imdbRating || "",
                                            tmdbId: root.itemData.tmdbId || "",
                                            imdbId: root.itemData.imdbId || ""
                                        }
                                        
                                        localLibraryService.addToLibrary(libraryItem)
                                    }
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
                        height: 1000
                        currentIndex: root.getStackLayoutIndex(root.currentTabIndex)
                        
                        // EPISODES Tab
                        Rectangle {
                            visible: root.isSeries()
                            color: "#09090b"
                            // ... (Episodes content same as before)
                            Text {
                                anchors.centerIn: parent
                                text: "Episodes content placeholder"
                                color: "#666"
                                visible: (root.itemData.numberOfSeasons || 0) === 0
                            }
                        }
                        
                        // CAST & CREW Tab
                        Rectangle {
                            color: "#09090b"
                            visible: true
                            Item {
                                width: parent.width - 100
                                height: 420
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.top: parent.top
                                anchors.topMargin: 20
                                
                                ListView {
                                    id: castListView
                                    anchors.fill: parent
                                    orientation: ListView.Horizontal
                                    spacing: 24
                                    clip: true
                                    model: castModel
                                    
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
                            Item {
                                width: parent.width - 100
                                height: 420
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.top: parent.top
                                anchors.topMargin: 20
                                
                                ListView {
                                    id: similarListView
                                    anchors.fill: parent
                                    orientation: ListView.Horizontal
                                    spacing: 24
                                    clip: true
                                    model: root.similarItems || []
                                    
                                    delegate: Item {
                                        width: 240
                                        height: 400
                                        property var itemData: modelData
                                        property bool isHovered: false
                                        
                                        MouseArea {
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            onEntered: parent.isHovered = true
                                            onExited: parent.isHovered = false
                                            onClicked: {
                                                if (parent.itemData && parent.itemData.id) {
                                                    root.loadDetails(parent.itemData.id, parent.itemData.type || "movie", "")
                                                }
                                            }
                                        }
                                        
                                        Column {
                                            anchors.fill: parent
                                            spacing: 12
                                            Item {
                                                width: parent.width; height: 360
                                                Image {
                                                    anchors.fill: parent
                                                    source: itemData.posterUrl || ""
                                                    fillMode: Image.PreserveAspectCrop
                                                    scale: isHovered ? 1.10 : 1.0
                                                    Behavior on scale { NumberAnimation { duration: 300 } }
                                                }
                                            }
                                            Text {
                                                width: parent.width
                                                text: itemData.title || itemData.name || ""
                                                color: "white"
                                                elide: Text.ElideRight
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        
                        // DETAILS Tab
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