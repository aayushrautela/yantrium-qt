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
    property var smartPlayState: ({})

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

            // Get smart play state
            console.log("[DetailScreen] Calling getSmartPlayState with details.type:", details.type, "tmdbId:", details.tmdbId, "id:", details.id)
            libraryService.getSmartPlayState(details)
        }
        function onSimilarItemsLoaded(items) {
            root.similarItems = items
        }
        function onSmartPlayStateLoaded(smartPlayState) {
            root.smartPlayState = smartPlayState
            console.log("[DetailScreen] Smart play state loaded:", JSON.stringify(smartPlayState))
        }
        function onError(message) {
            console.error("[DetailScreen] Error:", message)
            root.isLoading = false
        }
    }

    Connections {
        target: localLibraryService
        function onLibraryItemAdded(success) {
            if (success) {
                console.log("[DetailScreen] Item added to local library successfully")
                root.isInLibrary = true
                root.libraryChanged()
            }
        }
        function onLibraryItemRemoved(success) {
            console.log("[DetailScreen] onLibraryItemRemoved called - success:", success, "current isInLibrary:", root.isInLibrary)
            if (success) {
                console.log("[DetailScreen] Item removed from local library successfully - setting isInLibrary to false")
                // Update UI immediately - trust the removal operation result
                root.isInLibrary = false
                console.log("[DetailScreen] After setting false, isInLibrary is now:", root.isInLibrary)
                root.libraryChanged()
            } else {
                console.error("[DetailScreen] Failed to remove from local library")
                // Re-check state on failure to see current state
                checkIfInLibrary()
            }
        }
        function onIsInLibraryResult(inLibrary) {
            console.log("[DetailScreen] Local library check result:", inLibrary, "current isInLibrary:", root.isInLibrary)
            root.isInLibrary = inLibrary
            console.log("[DetailScreen] After library check, isInLibrary is now:", root.isInLibrary)
        }
    }

    Connections {
        target: traktWatchlistService
        function onWatchlistItemAdded(success) {
            if (success) {
                console.log("[DetailScreen] Item added to Trakt watchlist successfully")
                root.isInLibrary = true
                root.libraryChanged()
            }
        }
        function onWatchlistItemRemoved(success) {
            console.log("[DetailScreen] onWatchlistItemRemoved called - success:", success)
            if (success) {
                console.log("[DetailScreen] Item removed from Trakt watchlist successfully")
                // Update UI immediately - trust the removal operation result
                root.isInLibrary = false
                root.libraryChanged()
            } else {
                console.error("[DetailScreen] Failed to remove from Trakt watchlist")
                // Re-check state on failure to see current state
                checkIfInLibrary()
            }
        }
        function onIsInWatchlistResult(inWatchlist) {
            console.log("[DetailScreen] Trakt watchlist check result:", inWatchlist, "current isInLibrary:", root.isInLibrary)
            root.isInLibrary = inWatchlist
            console.log("[DetailScreen] After watchlist check, isInLibrary is now:", root.isInLibrary)
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
        // Always prefer imdbId for consistency - this is what's stored in the database
        var contentId = root.itemData.imdbId || root.itemData.id || ""
        if (!contentId) {
            console.log("[DetailScreen] checkIfInLibrary: No contentId available")
            console.log("[DetailScreen] itemData keys:", Object.keys(root.itemData))
            return
        }

        // Ensure we're using imdbId format if available (for consistency with database)
        if (root.itemData.imdbId && root.itemData.imdbId.startsWith("tt")) {
            contentId = root.itemData.imdbId
        }

        console.log("[DetailScreen] checkIfInLibrary called - contentId:", contentId, "authenticated:", traktAuthService.isAuthenticated, "imdbId:", root.itemData.imdbId, "id:", root.itemData.id, "current isInLibrary:", root.isInLibrary)

        if (traktAuthService.isAuthenticated) {
            // Check Trakt watchlist
            var type = root.itemData.type || "movie"
            if (type === "tv") {
                type = "show"  // UI displays as "show" but internally uses "tv"
            } else {
                type = "movie"
            }
            console.log("[DetailScreen] Checking Trakt watchlist - contentId:", contentId, "type:", type)
            traktWatchlistService.isInWatchlist(contentId, type)
        } else {
            // Check local library
            console.log("[DetailScreen] Checking local library - contentId:", contentId)
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
        if (root.itemData.type === "tv") {
            tabs.unshift("EPISODES")
        }
        return tabs
    }
    
    function isSeries() {
        return root.itemData.type === "tv"
    }
    
    function getStackLayoutIndex(tabIndex) {
        if (root.isSeries()) return tabIndex
        return tabIndex + 1
    }
    
    signal closeRequested()
    signal libraryChanged()
    
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
                            visible: root.itemData.type === "tv" && root.itemData.numberOfSeasons > 0
                            
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
                                            text: root.smartPlayState.buttonText || "Play"
                                            font.pixelSize: 18
                                            font.bold: true
                                            color: "#000000"
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                    }
                                }
                                onClicked: {
                                    var action = root.smartPlayState.action || "play"
                                    var season = root.smartPlayState.season || -1
                                    var episode = root.smartPlayState.episode || -1

                                    console.log("Play clicked for:", root.itemData.title || root.itemData.name,
                                               "action:", action, "S" + season + "E" + episode)

                                    // TODO: Implement actual playback logic based on action type
                                    if (action === "play") {
                                        console.log("Starting playback from beginning or specific episode")
                                    } else if (action === "continue") {
                                        console.log("Continuing playback from last position")
                                    } else if (action === "rewatch") {
                                        console.log("Starting rewatch from beginning")
                                    } else if (action === "soon") {
                                        console.log("Episode not yet available")
                                    }
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
                                        text: root.isInLibrary ? "In Library" : "Add to Library"
                                        font.pixelSize: 18
                                        color: "#ffffff"
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    }
                                }
                                onClicked: {
                                    // Always prefer imdbId for consistency - this is what's stored in the database
                                    var contentId = root.itemData.imdbId || root.itemData.id || ""
                                    var type = root.itemData.type || "movie"

                                    if (!contentId) {
                                        console.error("[DetailScreen] Cannot modify library: no contentId")
                                        console.error("[DetailScreen] itemData:", JSON.stringify(root.itemData))
                                        return
                                    }

                                    // Ensure we're using imdbId format if available (for consistency with database)
                                    if (root.itemData.imdbId && root.itemData.imdbId.startsWith("tt")) {
                                        contentId = root.itemData.imdbId
                                    }

                                    console.log("[DetailScreen] Library button clicked - contentId:", contentId, "type:", type, "isInLibrary:", root.isInLibrary, "imdbId:", root.itemData.imdbId, "id:", root.itemData.id)

                                    // Normalize type - use "tv" internally, UI displays as "show"
                                    if (type === "tv") {
                                        type = "show"
                                    } else {
                                        type = "movie"
                                    }

                                    // Toggle: add if not in library, remove if already in library
                                    if (root.isInLibrary) {
                                        // Remove from library
                                        console.log("[DetailScreen] Removing from library - contentId:", contentId, "type:", type, "isInLibrary:", root.isInLibrary)

                                        if (traktAuthService.isAuthenticated) {
                                            traktWatchlistService.removeFromWatchlist(type, contentId)
                                        } else {
                                            console.log("[DetailScreen] Calling removeFromLibrary with contentId:", contentId)
                                            localLibraryService.removeFromLibrary(contentId)
                                        }
                                    } else {
                                        // Add to library
                                        console.log("[DetailScreen] Adding to library - contentId:", contentId, "type:", type)

                                        if (traktAuthService.isAuthenticated) {
                                            traktWatchlistService.addToWatchlist(type, contentId)
                                        } else {
                                            var libraryItem = {
                                                contentId: contentId,
                                                type: type === "show" ? "tv" : "movie",  // Use "tv" internally to match database
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

                                            console.log("[DetailScreen] Calling addToLibrary with item - contentId:", libraryItem.contentId)
                                            localLibraryService.addToLibrary(libraryItem)
                                        }
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
                                        width: 240  // Match LibraryScreen catalog card width
                                        height: 400  // Match LibraryScreen catalog card height

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
                                            cursorShape: Qt.PointingHandCursor
                                        }

                                        Column {
                                            anchors.fill: parent
                                            spacing: 12

                                            // Poster (exactly like LibraryScreen catalog cards)
                                            Item {
                                                id: imageContainer
                                                width: parent.width
                                                height: 360

                                                Rectangle { id: maskShape; anchors.fill: parent; radius: 8; visible: false }
                                                layer.enabled: true
                                                layer.effect: OpacityMask { maskSource: maskShape }

                                                Rectangle { anchors.fill: parent; color: "#2a2a2a"; radius: 8 }

                                                Image {
                                                    id: img
                                                    anchors.fill: parent
                                                    source: itemData.posterUrl || ""
                                                    fillMode: Image.PreserveAspectCrop
                                                    asynchronous: true
                                                    smooth: true
                                                    scale: isHovered ? 1.10 : 1.0
                                                    Behavior on scale { NumberAnimation { duration: 300; easing.type: Easing.OutQuad } }
                                                }

                                                // Rating overlay (exactly like LibraryScreen)
                                                Rectangle {
                                                    anchors.bottom: parent.bottom; anchors.right: parent.right; anchors.margins: 8
                                                    visible: itemData.rating !== undefined && itemData.rating !== ""
                                                    color: "black"
                                                    opacity: 0.8
                                                    radius: 4
                                                    width: 40
                                                    height: 20
                                                    Row {
                                                        anchors.centerIn: parent; spacing: 2
                                                        Text { text: "\u2605"; color: "#ffffff"; font.pixelSize: 10 }
                                                        Text { text: itemData.rating || ""; color: "white"; font.pixelSize: 10; font.bold: true }
                                                    }
                                                }

                                                // Border (exactly like LibraryScreen)
                                                Rectangle {
                                                    anchors.fill: parent; color: "transparent"; radius: 8; z: 10
                                                    border.width: 3; border.color: isHovered ? "#ffffff" : "transparent"
                                                    Behavior on border.color { ColorAnimation { duration: 200 } }
                                                }
                                            }

                                            // Title (exactly like LibraryScreen - centered below poster)
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