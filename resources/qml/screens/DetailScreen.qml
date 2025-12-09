import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import Yantrium.Services 1.0
import "../components"

Item {
    id: root
    
    property var itemData: ({})  // QVariantMap from parent
    
    // All services are singletons, accessed directly
    property LibraryService libraryService: LibraryService
    property TraktAuthService traktAuthService: TraktAuthService
    property LocalLibraryService localLibrary: LocalLibraryService
    property TraktWatchlistService traktWatchlist: TraktWatchlistService
    property NavigationService navigationService: NavigationService
    
    property bool isInLibrary: false
    property var smartPlayState: ({})
    property string currentRequestContentId: ""  // Track current request to ignore stale responses

    Connections {
        target: libraryService
        function onItemDetailsLoaded(details) {
            // Check if this response matches the current request (ignore stale responses)
            // Compare both id and imdbId fields since contentId could be either format
            var responseId = details.id || ""
            var responseImdbId = details.imdbId || ""
            
            if (root.currentRequestContentId) {
                // Match if response id or imdbId matches the request
                var matches = (responseId === root.currentRequestContentId) || 
                             (responseImdbId === root.currentRequestContentId)
                
                if (!matches) {
                    console.log("[DetailScreen] Ignoring stale response - expected:", root.currentRequestContentId, 
                               "got id:", responseId, "imdbId:", responseImdbId)
                    return
                }
            }
            
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
            
            // Initialize seasons list for TV shows
            if (details.type === "tv") {
                var numSeasons = details.numberOfSeasons || 1
                var seasons = []
                for (var i = 1; i <= numSeasons; i++) {
                    seasons.push(i)
                }
                root.availableSeasons = seasons
                root.selectedSeasonNumber = 1
                // Load first season episodes - use whatever ID is available (prefer IMDB, then TMDB, then contentId)
                var contentId = details.imdbId || details.id || ""
                if (!contentId && details.tmdbId) {
                    // Fallback to TMDB ID if no IMDB/contentId available
                    var tmdbId = parseInt(details.tmdbId) || details.tmdbId
                    libraryService.loadSeasonEpisodes(tmdbId, 1)
                } else if (contentId) {
                    libraryService.loadSeasonEpisodes(contentId, 1)
                }
            }
        }
        function onSimilarItemsLoaded(items) {
            root.similarItems = items
        }
        function onSmartPlayStateLoaded(smartPlayState) {
            root.smartPlayState = smartPlayState
            console.log("[DetailScreen] Smart play state loaded:", JSON.stringify(smartPlayState))
        }
        function onSeasonEpisodesLoaded(seasonNumber, episodes) {
            if (seasonNumber === root.selectedSeasonNumber) {
                root.currentSeasonEpisodes = episodes
                console.log("[DetailScreen] Episodes loaded for season", seasonNumber, "count:", episodes.length)
            }
        }
        function onError(message) {
            console.error("[DetailScreen] Error:", message)
            root.isLoading = false
            // Clear request tracking on error so we can retry
            root.currentRequestContentId = ""
        }
    }

    Connections {
        target: localLibrary
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
        target: traktWatchlist
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
            traktWatchlist.isInWatchlist(contentId, type)
        } else {
            // Check local library
            console.log("[DetailScreen] Checking local library - contentId:", contentId)
            localLibrary.isInLibrary(contentId)
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
    property var availableSeasons: []
    property var currentSeasonEpisodes: []
    property int selectedSeasonNumber: 1
    
    // Episode mode properties
    property bool isEpisodeMode: {
        var episodeNum = root.itemData.episodeNumber || root.itemData.episode || 0
        return episodeNum > 0
    }
    property var showData: ({})  // Store show information for "Back to Series" navigation
    
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
        // For episode mode, only show EPISODES tab
        if (root.isEpisodeMode) {
            return ["EPISODES"]
        }
        // For regular shows/movies
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
    signal playRequested(string streamUrl, var contentData)
    signal showRequested(string contentId, string type, string addonId)  // For navigating back to show from episode
    
    function loadDetails(contentId, type, addonId) {
        console.log("[DetailScreen] loadDetails called - contentId:", contentId, "type:", type, "addonId:", addonId)
        if (!contentId || !type) {
            console.error("[DetailScreen] Missing contentId or type - contentId:", contentId, "type:", type)
            return
        }
        
        // Track this request to ignore stale responses
        root.currentRequestContentId = contentId
        
        // Clear old data to prevent showing stale content
        root.itemData = {}
        root.similarItems = []
        root.smartPlayState = {}
        root.currentSeasonEpisodes = []
        root.isInLibrary = false
        root.showData = {}  // Clear preserved show data when loading new content
        
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
            
            source: {
                // For episodes, prefer show backdrop first, then episode thumbnail as fallback
                if (root.isEpisodeMode) {
                    return root.itemData.backdropUrl || root.itemData.thumbnailUrl || root.itemData.background || root.itemData.showBackdropUrl || root.itemData.showPosterUrl || ""
                }
                return root.itemData.backdropUrl || root.itemData.background || ""
            }
            
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
        
        Flickable {
            anchors.fill: parent
            contentWidth: width
            contentHeight: contentColumn.height
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            z: 2

            Column {
                id: contentColumn
                width: parent.width
                spacing: 0
                
                // Backdrop Section
                Item {
                    width: parent.width
                    height: Math.max(400, backdropContent.height + 150)  // Dynamic height based on content
                    
                    // Gradient Overlay - Full screen
                    Rectangle {
                        anchors.fill: parent
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#00000000" }
                            GradientStop { position: 0.5; color: "#8009090b" }
                            GradientStop { position: 1.0; color: "#ff09090b" }
                        }
                    }
                    
                    // Title and Info Section (overlay on backdrop)
                    Column {
                        id: backdropContent
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.topMargin: 100  // Start from top instead of center
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
                                source: {
                                    // For episodes, prefer show logo, fallback to episode logo
                                    if (root.isEpisodeMode) {
                                        return root.itemData.showLogoUrl || root.itemData.logoUrl || root.itemData.logo || ""
                                    }
                                    return root.itemData.logoUrl || root.itemData.logo || ""
                                }
                                visible: {
                                    var logoUrl = root.isEpisodeMode ? 
                                        (root.itemData.showLogoUrl || root.itemData.logoUrl || root.itemData.logo || "") :
                                        (root.itemData.logoUrl || root.itemData.logo || "")
                                    return logoUrl !== "" && logoImage.status === Image.Ready
                                }
                                asynchronous: true
                            }
                            
                            Text {
                                anchors.left: parent.left
                                anchors.top: parent.top
                                text: {
                                    // For episodes, show the show title, otherwise show the item title
                                    if (root.isEpisodeMode) {
                                        return root.itemData.showTitle || root.itemData.title || root.itemData.name || ""
                                    }
                                    return root.itemData.title || root.itemData.name || ""
                                }
                                font.pixelSize: 80
                                font.bold: true
                                color: "#ff0000"
                                // Only show text if logo is missing (no URL) or failed to load
                                visible: (root.itemData.logoUrl || root.itemData.logo) === "" || logoImage.status === Image.Error
                            }
                        }
                        
                        // Episode Title (for episode mode)
                        Text {
                            width: Math.min(parent.width, 800)
                            text: {
                                if (root.isEpisodeMode) {
                                    var epNum = root.itemData.episodeNumber || root.itemData.episode || 0
                                    var epTitle = root.itemData.episodeTitle || root.itemData.title || ""
                                    return "Episode " + epNum + (epTitle ? ": " + epTitle : "")
                                }
                                return ""
                            }
                            color: "#ffffff"
                            font.pixelSize: 32
                            font.bold: true
                            visible: root.isEpisodeMode && text !== ""
                            wrapMode: Text.WordWrap
                        }
                        
                        // Row 1: For episodes: Air Date • Runtime | For movies/shows: Release Date • Content Rating • Runtime • Genres
                        Row {
                            spacing: 8

                            // Episode mode: Formatted Metadata Line
                            Text {
                                text: root.itemData.metadataLine || ""
                                color: "#ffffff"
                                font.pixelSize: 18
                                visible: root.isEpisodeMode && text !== ""
                            }

                            // Movie/Show mode: Build metadata with dots only between non-empty items
                            Repeater {
                                model: {
                                    if (root.isEpisodeMode) return []
                                    
                                    var parts = []
                                    var date = root.itemData.releaseDate || root.itemData.firstAirDate || ""
                                    var rating = root.itemData.contentRating || ""
                                    var runtime = (root.itemData.type === "movie") ? (root.itemData.runtimeFormatted || "") : ""
                                    var genres = root.itemData.genres || []
                                    
                                    if (date !== "") parts.push({text: date, isDot: false})
                                    if (rating !== "") parts.push({text: rating, isDot: false})
                                    if (runtime !== "") parts.push({text: runtime, isDot: false})
                                    if (genres.length > 0) {
                                        var limitedGenres = []
                                        for (var i = 0; i < Math.min(3, genres.length); i++) {
                                            if (genres[i]) limitedGenres.push(genres[i])
                                        }
                                        if (limitedGenres.length > 0) {
                                            parts.push({text: limitedGenres.join(" "), isDot: false})
                                        }
                                    }
                                    
                                    // Insert dots between items
                                    var result = []
                                    for (var j = 0; j < parts.length; j++) {
                                        if (j > 0) result.push({text: "•", isDot: true})
                                        result.push(parts[j])
                                    }
                                    return result
                                }
                                
                                Text {
                                    text: modelData.text || ""
                                    color: "#ffffff"
                                    font.pixelSize: 18
                                    visible: text !== ""
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
                        
                        // Row 3: Rating Scores (hidden for episodes)
                        Row {
                            spacing: 20
                            visible: !root.isEpisodeMode
                            
                            // IMDb Score
                            Row {
                                spacing: 6
                                visible: {
                                    var rating = root.itemData.imdbRating
                                    return rating !== undefined && rating !== null && rating !== ""
                                }
                                
                                Image {
                                    id: imdbImage
                                    width: 40
                                    height: 20
                                    source: "qrc:/assets/icons/imdb.svg"
                                    sourceSize.width: 80
                                    sourceSize.height: 40
                                    mipmap: true
                                    asynchronous: true
                                    fillMode: Image.PreserveAspectFit
                                    anchors.verticalCenter: parent.verticalCenter
                                    smooth: true
                                    antialiasing: true
                                    
                                    onStatusChanged: {
                                        if (status === Image.Error) {
                                            console.error("[DetailScreen] Failed to load IMDb logo:", source)
                                        } else if (status === Image.Ready) {
                                            console.log("[DetailScreen] IMDb logo loaded successfully")
                                        }
                                    }
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
                                    // Only show when OMDB data is available
                                    var omdbRatings = root.itemData.omdbRatings
                                    if (!omdbRatings || !Array.isArray(omdbRatings) || omdbRatings.length === 0) {
                                        return false
                                    }
                                    var score = root.itemData.metascore
                                    return score !== undefined && score !== null && score !== ""
                                }
                                
                                Image {
                                    id: metacriticImage
                                    width: 20
                                    height: 20
                                    source: "qrc:/assets/icons/metacritic.svg"
                                    sourceSize.width: 40
                                    sourceSize.height: 40
                                    mipmap: true
                                    asynchronous: true
                                    fillMode: Image.PreserveAspectFit
                                    anchors.verticalCenter: parent.verticalCenter
                                    smooth: true
                                    antialiasing: true
                                    
                                    onStatusChanged: {
                                        if (status === Image.Error) {
                                            console.error("[DetailScreen] Failed to load Metacritic logo:", source)
                                        } else if (status === Image.Ready) {
                                            console.log("[DetailScreen] Metacritic logo loaded successfully")
                                        }
                                    }
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
                                        id: rtImage
                                        width: 20
                                        height: 20
                                        source: "qrc:/assets/icons/rotten_tomatoes.svg"
                                        sourceSize.width: 40
                                        sourceSize.height: 40
                                        mipmap: true
                                        asynchronous: true
                                        fillMode: Image.PreserveAspectFit
                                        anchors.verticalCenter: parent.verticalCenter
                                        smooth: true
                                        antialiasing: true
                                        
                                        onStatusChanged: {
                                            if (status === Image.Error) {
                                                console.error("[DetailScreen] Failed to load Rotten Tomatoes logo:", source)
                                            } else if (status === Image.Ready) {
                                                console.log("[DetailScreen] Rotten Tomatoes logo loaded successfully")
                                            }
                                        }
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
                                width: 160
                                height: 52
                                scale: hovered ? 1.05 : 1.0
                                Behavior on scale { NumberAnimation { duration: 150; easing.type: Easing.OutQuad } }
                                background: Rectangle {
                                    color: "#ffffff"
                                    radius: 26  // Pill shape (half of height)
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
                                               "action:", action, "type:", root.itemData.type)

                                    // Format episode ID for TV shows only (not for movies)
                                    var episodeId = ""
                                    if (root.itemData.type === "tv" && season > 0 && episode > 0) {
                                        episodeId = "S" + String(season).padStart(2, '0') + "E" + String(episode).padStart(2, '0')
                                        console.log("TV show - formatted episode ID:", episodeId)
                                    }

                                    // Open stream selection dialog
                                    streamDialog.loadStreams(root.itemData, episodeId)
                                    streamDialog.open()
                                }
                            }
                            
                            Button {
                                width: 160
                                height: 52
                                scale: hovered ? 1.05 : 1.0
                                Behavior on scale { NumberAnimation { duration: 150; easing.type: Easing.OutQuad } }
                                background: Rectangle {
                                    color: "#40000000"
                                    border.width: 1
                                    border.color: "#ffffff"
                                    radius: 26
                                }
                                contentItem: Item {
                                    Row {
                                        anchors.centerIn: parent
                                        spacing: 8

                                        Image {
                                            visible: root.isEpisodeMode
                                            source: "qrc:/assets/icons/left_catalog.svg"
                                            width: 16
                                            height: 16
                                            fillMode: Image.PreserveAspectFit
                                        }

                                        Text {
                                            text: root.isEpisodeMode ? "Back to Series" : (root.isInLibrary ? "✓ Library" : "+ Library")
                                            font.pixelSize: 16
                                            color: "#ffffff"
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                        }
                                    }
                                }
                                onClicked: {
                                    if (root.isEpisodeMode) {
                                        // Navigate back to show
                                        var showId = root.itemData.showImdbId || root.itemData.showId || root.itemData.imdbId || ""
                                        var showType = "tv"
                                        var addonId = root.itemData.addonId || ""
                                        
                                        console.log("[DetailScreen] Back to Series clicked - showId:", showId)
                                        
                                        if (showId) {
                                            root.showRequested(showId, showType, addonId)
                                        } else {
                                            console.error("[DetailScreen] Cannot navigate to show: no showId available")
                                        }
                                        return
                                    }
                                    
                                    // Original library toggle logic for non-episodes
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
                                            traktWatchlist.removeFromWatchlist(type, contentId)
                                        } else {
                                            console.log("[DetailScreen] Calling removeFromLibrary with contentId:", contentId)
                                            localLibrary.removeFromLibrary(contentId)
                                        }
                                    } else {
                                        // Add to library
                                        console.log("[DetailScreen] Adding to library - contentId:", contentId, "type:", type)

                                        if (traktAuthService.isAuthenticated) {
                                            traktWatchlist.addToWatchlist(type, contentId)
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
                                            localLibrary.addToLibrary(libraryItem)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                
                // Tabs Section (show for episodes too, but only EPISODES tab)
                Column {
                    width: parent.width
                    spacing: 0
                    visible: true
                    
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
                        height: {
                            switch (currentIndex) {
                                case 0: // EPISODES (for TV shows or episodes)
                                    return (root.isSeries() || root.isEpisodeMode) ? Math.max(400, episodesListView.contentHeight + 80) : 0
                                case 1: // CAST & CREW
                                    return 420
                                case 2: // MORE LIKE THIS
                                    return 420
                                case 3: // DETAILS
                                    return 100
                                default:
                                    return 400
                            }
                        }
                        currentIndex: root.getStackLayoutIndex(root.currentTabIndex)
                        
                        // EPISODES Tab
                        Rectangle {
                            visible: root.isSeries() || root.isEpisodeMode
                            color: "#09090b"
                            
                            Item {
                                anchors.fill: parent
                                anchors.leftMargin: 50
                                anchors.rightMargin: 50
                                anchors.topMargin: 30
                                
                                Column {
                                    anchors.fill: parent
                                    spacing: 20
                                    
                                    // Season selector and episode count
                                    Row {
                                        spacing: 20
                                    
                                    // Season dropdown
                                    ComboBox {
                                        id: seasonComboBox
                                        width: 200
                                        height: 40
                                        model: root.availableSeasons
                                        currentIndex: {
                                            var idx = root.availableSeasons.indexOf(root.selectedSeasonNumber)
                                            return idx >= 0 ? idx : 0
                                        }
                                        visible: root.availableSeasons.length > 0
                                        
                                        background: Rectangle {
                                            color: "#2d2d2d"
                                            radius: 4
                                        }
                                        
                                        contentItem: Row {
                                            anchors.left: parent.left
                                            anchors.leftMargin: 12
                                            anchors.verticalCenter: parent.verticalCenter
                                            spacing: 8
                                            
                                            Text {
                                                anchors.verticalCenter: parent.verticalCenter
                                                text: "Season " + (root.availableSeasons[seasonComboBox.currentIndex] || root.selectedSeasonNumber)
                                                color: "#ffffff"
                                                font.pixelSize: 16
                                            }
                                            
                                            Item {
                                                width: 16
                                                height: 16
                                                anchors.verticalCenter: parent.verticalCenter
                                                
                                                Text {
                                                    anchors.centerIn: parent
                                                    text: "▼"
                                                    color: "#ffffff"
                                                    font.pixelSize: 10
                                                }
                                            }
                                        }
                                        
                                        popup: Popup {
                                            y: seasonComboBox.height
                                            width: seasonComboBox.width
                                            implicitHeight: contentItem.implicitHeight
                                            padding: 4
                                            
                                            contentItem: ListView {
                                                clip: true
                                                implicitHeight: contentHeight
                                                model: seasonComboBox.popup.visible ? seasonComboBox.delegateModel : null
                                                currentIndex: seasonComboBox.highlightedIndex
                                                
                                                ScrollIndicator.vertical: ScrollIndicator { }
                                            }
                                            
                                            background: Rectangle {
                                                color: "#2d2d2d"
                                                radius: 4
                                            }
                                        }
                                        
                                        delegate: ItemDelegate {
                                            width: seasonComboBox.width
                                            height: 40
                                            
                                            background: Rectangle {
                                                color: parent.hovered ? "#3d3d3d" : "#2d2d2d"
                                            }
                                            
                                            contentItem: Text {
                                                text: "Season " + modelData
                                                color: "#ffffff"
                                                font.pixelSize: 16
                                                anchors.left: parent.left
                                                anchors.leftMargin: 12
                                                anchors.verticalCenter: parent.verticalCenter
                                            }
                                        }
                                        
                                        onActivated: function(index) {
                                            var newSeason = root.availableSeasons[index]
                                            if (newSeason !== root.selectedSeasonNumber) {
                                                root.selectedSeasonNumber = newSeason
                                                root.currentSeasonEpisodes = [] // Clear while loading
                                                // Use whatever ID is available (prefer IMDB, then TMDB, then contentId)
                                                var contentId = root.itemData.imdbId || root.itemData.id || ""
                                                if (!contentId && root.itemData.tmdbId) {
                                                    // Fallback to TMDB ID if no IMDB/contentId available
                                                    var tmdbId = parseInt(root.itemData.tmdbId) || root.itemData.tmdbId
                                                    libraryService.loadSeasonEpisodes(tmdbId, newSeason)
                                                } else if (contentId) {
                                                    libraryService.loadSeasonEpisodes(contentId, newSeason)
                                                }
                                            }
                                        }
                                    }
                                    
                                    // Episode count
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: root.currentSeasonEpisodes.length + " Episodes"
                                        color: "#ffffff"
                                        font.pixelSize: 16
                                    }
                                    }
                                
                                // Episode cards list
                                Item {
                                    width: parent.width
                                    height: Math.max(300, episodesListView.contentHeight)

                                    ListView {
                                        id: episodesListView
                                        anchors.fill: parent
                                        orientation: ListView.Horizontal
                                        spacing: 20
                                        model: root.currentSeasonEpisodes
                                        clip: true

                                        PropertyAnimation {
                                            id: episodeScrollAnimation
                                            target: episodesListView
                                            property: "contentX"
                                            duration: 300
                                            easing.type: Easing.OutCubic
                                        }
                                        
                                        delegate: Item {
                                            width: 320
                                            height: 300  // Fixed height for episode cards
                                            
                                            Column {
                                                width: parent.width
                                                spacing: 12
                                                
                                                // Thumbnail
                                                Item {
                                                    width: parent.width
                                                    height: 180

                                                    property bool isHovered: false

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
                                                        anchors.fill: parent
                                                        source: modelData.thumbnailUrl || ""
                                                        fillMode: Image.PreserveAspectCrop
                                                        asynchronous: true
                                                        smooth: true
                                                        scale: parent.isHovered ? 1.10 : 1.0
                                                        Behavior on scale { NumberAnimation { duration: 300; easing.type: Easing.OutQuad } }
                                                    }

                                                    MouseArea {
                                                        anchors.fill: parent
                                                        hoverEnabled: true
                                                        onEntered: parent.isHovered = true
                                                        onExited: parent.isHovered = false
                                                        onClicked: {
                                                            // Load episode details
                                                            var episodeData = modelData
                                                            // Use preserved showData if in episode mode, otherwise use current itemData
                                                            var showData = root.isEpisodeMode && Object.keys(root.showData).length > 0 ? 
                                                                           root.showData : root.itemData
                                                            
                                                            // Create episode item data with show information
                                                            var episodeItemData = {
                                                                // Episode info
                                                                episodeNumber: episodeData.episodeNumber || 0,
                                                                episode: episodeData.episodeNumber || 0,
                                                                episodeTitle: episodeData.title || "",
                                                                title: episodeData.title || "",
                                                                description: episodeData.description || episodeData.overview || "",
                                                                airDate: episodeData.airDate || "",
                                                                metadataLine: episodeData.metadataLine || "",
                                                                duration: episodeData.duration || 0,
                                                                thumbnailUrl: episodeData.thumbnailUrl || "",
                                                                
                                                                // Show info (for backdrop and navigation)
                                                                showId: showData.id || "",
                                                                showImdbId: showData.imdbId || "",
                                                                showTitle: showData.title || showData.name || "",
                                                                showPosterUrl: showData.posterUrl || "",
                                                                showBackdropUrl: showData.backdropUrl || showData.background || "",
                                                                showLogoUrl: showData.logoUrl || showData.logo || "",
                                                                showTmdbId: showData.tmdbId || "",
                                                                showNumberOfSeasons: showData.numberOfSeasons || 1,
                                                                
                                                                // Season info
                                                                season: root.selectedSeasonNumber || 1,
                                                                seasonNumber: root.selectedSeasonNumber || 1,
                                                                
                                                                // Type and metadata
                                                                type: "tv",
                                                                addonId: showData.addonId || "",
                                                                
                                                                // For smart play state
                                                                tmdbId: showData.tmdbId || "",
                                                                id: showData.id || "",
                                                                imdbId: showData.imdbId || ""
                                                            }
                                                            
                                                            // Preserve availableSeasons if not already set
                                                            if (root.availableSeasons.length === 0 && episodeItemData.showNumberOfSeasons > 0) {
                                                                var seasons = []
                                                                for (var i = 1; i <= episodeItemData.showNumberOfSeasons; i++) {
                                                                    seasons.push(i)
                                                                }
                                                                root.availableSeasons = seasons
                                                            }
                                                            
                                                            // Set smart play state for episode
                                                            episodeItemData.smartPlayState = {
                                                                action: "play",
                                                                season: root.selectedSeasonNumber || 1,
                                                                episode: episodeData.episodeNumber || 0,
                                                                buttonText: "Play"
                                                            }
                                                            
                                                            console.log("[DetailScreen] Loading episode details:", episodeItemData.episodeNumber, episodeItemData.episodeTitle)
                                                            
                                                            // Preserve show data if not already preserved (first time entering episode mode)
                                                            if (Object.keys(root.showData).length === 0) {
                                                                root.showData = showData
                                                            }
                                                            
                                                            // Set itemData directly for episode mode
                                                            root.itemData = episodeItemData
                                                            root.isLoading = false
                                                            
                                                            // Set smart play state
                                                            root.smartPlayState = episodeItemData.smartPlayState || {
                                                                action: "play",
                                                                season: root.selectedSeasonNumber || 1,
                                                                episode: episodeData.episodeNumber || 0,
                                                                buttonText: "Play"
                                                            }
                                                            
                                                            // Set current tab to EPISODES (index 0) for episode mode
                                                            root.currentTabIndex = 0
                                                            
                                                            // Don't check library for episodes
                                                            root.isInLibrary = false
                                                            
                                                            // Clear cast and similar items for episodes
                                                            root.similarItems = []
                                                            castModel.clear()
                                                            
                                                            // Ensure episodes are loaded for the current season
                                                            // Use whatever ID is available (prefer IMDB, then TMDB, then contentId)
                                                            var showContentId = root.itemData.showImdbId || root.itemData.showId || ""
                                                            if (!showContentId && root.itemData.showTmdbId) {
                                                                // Fallback to TMDB ID if no IMDB/contentId available
                                                                var tmdbId = parseInt(root.itemData.showTmdbId) || root.itemData.showTmdbId
                                                                var seasonNum = root.selectedSeasonNumber || 1
                                                                libraryService.loadSeasonEpisodes(tmdbId, seasonNum)
                                                            } else if (showContentId) {
                                                                var seasonNum = root.selectedSeasonNumber || 1
                                                                libraryService.loadSeasonEpisodes(showContentId, seasonNum)
                                                            }
                                                        }
                                                        cursorShape: Qt.PointingHandCursor
                                                    }
                                                    
                                                    // Duration badge
                                                    Rectangle {
                                                        anchors.bottom: parent.bottom
                                                        anchors.right: parent.right
                                                        anchors.margins: 8
                                                        width: durationText.width + 12
                                                        height: 24
                                                        color: "#2d2d2d"
                                                        opacity: 0.9
                                                        radius: 4
                                                        
                                                        Text {
                                                            id: durationText
                                                            anchors.centerIn: parent
                                                            text: (modelData.duration || 0) + "m"
                                                            color: "#ffffff"
                                                            font.pixelSize: 12
                                                            font.bold: true
                                                        }
                                                    }
                                                }
                                                
                                                // Episode number and title
                                                Text {
                                                    width: parent.width
                                                    text: "E" + modelData.episodeNumber + ": " + (modelData.title || "")
                                                    color: "#ffffff"
                                                    font.pixelSize: 16
                                                    font.bold: true
                                                    elide: Text.ElideRight
                                                }
                                                
                                                // Metadata line (formatted in backend: "Release Date • Runtime")
                                                Text {
                                                    width: parent.width
                                                    text: modelData.metadataLine || ""
                                                    color: "#aaaaaa"
                                                    font.pixelSize: 14
                                                    visible: (modelData.metadataLine || "") !== ""
                                                    elide: Text.ElideRight
                                                }
                                                
                                                // Description
                                                Text {
                                                    width: parent.width
                                                    text: modelData.description || ""
                                                    color: "#aaaaaa"
                                                    font.pixelSize: 14
                                                    wrapMode: Text.WordWrap
                                                    maximumLineCount: 3
                                                    elide: Text.ElideRight
                                                    lineHeight: 1.3
                                                }
                                            }
                                        }

                                        // Left arrow
                                        Item {
                                            anchors.left: parent.left
                                            anchors.top: parent.top
                                            anchors.topMargin: 90  // Center on thumbnail area (180/2)
                                            width: 48
                                            height: 48
                                            visible: episodesListView.contentX > 0
                                            z: 10

                                            property bool isHovered: false

                                            Item {
                                                anchors.centerIn: parent
                                                width: 48
                                                height: 48
                                                opacity: parent.isHovered ? 1.0 : 0.4
                                                Behavior on opacity { NumberAnimation { duration: 200 } }
                                                
                                                Image {
                                                    id: episodesLeftArrowIcon
                                                    anchors.centerIn: parent
                                                    
                                                    source: "qrc:/assets/icons/left_catalog.svg"
                                                    
                                                    // Large source + mipmap ensures smooth downscaling
                                                    sourceSize.width: 128
                                                    sourceSize.height: 128
                                                    mipmap: true
                                                    smooth: true
                                                    antialiasing: true
                                                    
                                                    width: 48
                                                    height: 48
                                                    fillMode: Image.PreserveAspectFit
                                                }
                                            }

                                            MouseArea {
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                onEntered: parent.isHovered = true
                                                onExited: parent.isHovered = false
                                                onClicked: {
                                                    var targetX = Math.max(0, episodesListView.contentX - episodesListView.width * 0.8)
                                                    episodeScrollAnimation.to = targetX
                                                    episodeScrollAnimation.start()
                                                }
                                            }
                                        }

                                        // Right arrow
                                        Item {
                                            anchors.right: parent.right
                                            anchors.top: parent.top
                                            anchors.topMargin: 90  // Center on thumbnail area (180/2)
                                            width: 48
                                            height: 48
                                            visible: episodesListView.contentX < episodesListView.contentWidth - episodesListView.width
                                            z: 10

                                            property bool isHovered: false

                                            Item {
                                                anchors.centerIn: parent
                                                width: 48
                                                height: 48
                                                opacity: parent.isHovered ? 1.0 : 0.4
                                                Behavior on opacity { NumberAnimation { duration: 200 } }
                                                
                                                Image {
                                                    id: episodesRightArrowIcon
                                                    anchors.centerIn: parent
                                                    
                                                    source: "qrc:/assets/icons/right_catalog.svg"
                                                    
                                                    // Large source + mipmap ensures smooth downscaling
                                                    sourceSize.width: 128
                                                    sourceSize.height: 128
                                                    mipmap: true
                                                    smooth: true
                                                    antialiasing: true
                                                    
                                                    width: 48
                                                    height: 48
                                                    fillMode: Image.PreserveAspectFit
                                                }
                                            }

                                            MouseArea {
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                onEntered: parent.isHovered = true
                                                onExited: parent.isHovered = false
                                                onClicked: {
                                                    var targetX = Math.min(
                                                        episodesListView.contentWidth - episodesListView.width,
                                                        episodesListView.contentX + episodesListView.width * 0.8
                                                    )
                                                    episodeScrollAnimation.to = targetX
                                                    episodeScrollAnimation.start()
                                                }
                                            }
                                        }
                                    }
                                }
                                }
                            }
                            
                            // Empty state
                            Text {
                                anchors.centerIn: parent
                                text: "No episodes available"
                                color: "#666666"
                                font.pixelSize: 16
                                visible: root.currentSeasonEpisodes.length === 0 && !root.isLoading
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
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 48
                                    height: 48
                                    visible: castListView.contentX > 0
                                    z: 10
                                    
                                    property bool isHovered: false
                                    
                                    Item {
                                        anchors.centerIn: parent
                                        width: 48
                                        height: 48
                                        opacity: parent.isHovered ? 1.0 : 0.4
                                        Behavior on opacity { NumberAnimation { duration: 200 } }
                                        
                                        Image {
                                            id: castLeftArrowIcon
                                            anchors.centerIn: parent
                                            
                                            source: "qrc:/assets/icons/left_catalog.svg"
                                            
                                            // Large source + mipmap ensures smooth downscaling
                                            sourceSize.width: 128
                                            sourceSize.height: 128
                                            mipmap: true
                                            smooth: true
                                            antialiasing: true
                                            
                                            width: 48
                                            height: 48
                                            fillMode: Image.PreserveAspectFit
                                        }
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
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 48
                                    height: 48
                                    visible: castListView.contentX < castListView.contentWidth - castListView.width
                                    z: 10
                                    
                                    property bool isHovered: false
                                    
                                    Item {
                                        anchors.centerIn: parent
                                        width: 48
                                        height: 48
                                        opacity: parent.isHovered ? 1.0 : 0.4
                                        Behavior on opacity { NumberAnimation { duration: 200 } }
                                        
                                        Image {
                                            id: castRightArrowIcon
                                            anchors.centerIn: parent
                                            
                                            source: "qrc:/assets/icons/right_catalog.svg"
                                            
                                            // Large source + mipmap ensures smooth downscaling
                                            sourceSize.width: 128
                                            sourceSize.height: 128
                                            mipmap: true
                                            smooth: true
                                            antialiasing: true
                                            
                                            width: 48
                                            height: 48
                                            fillMode: Image.PreserveAspectFit
                                        }
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
                                    
                                    PropertyAnimation {
                                        id: similarScrollAnimation
                                        target: similarListView
                                        property: "contentX"
                                        duration: 300
                                        easing.type: Easing.OutCubic
                                    }
                                    
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
                                
                                // Left arrow
                                Item {
                                    anchors.left: parent.left
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 48
                                    height: 48
                                    visible: similarListView.contentX > 0
                                    z: 10
                                    
                                    property bool isHovered: false
                                    
                                    Item {
                                        anchors.centerIn: parent
                                        width: 48
                                        height: 48
                                        opacity: parent.isHovered ? 1.0 : 0.4
                                        Behavior on opacity { NumberAnimation { duration: 200 } }
                                        
                                        Image {
                                            id: similarLeftArrowIcon
                                            anchors.centerIn: parent
                                            
                                            source: "qrc:/assets/icons/left_catalog.svg"
                                            
                                            // Large source + mipmap ensures smooth downscaling
                                            sourceSize.width: 128
                                            sourceSize.height: 128
                                            mipmap: true
                                            smooth: true
                                            antialiasing: true
                                            
                                            width: 48
                                            height: 48
                                            fillMode: Image.PreserveAspectFit
                                        }
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
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 48
                                    height: 48
                                    visible: similarListView.contentX < similarListView.contentWidth - similarListView.width
                                    z: 10
                                    
                                    property bool isHovered: false
                                    
                                    Item {
                                        anchors.centerIn: parent
                                        width: 48
                                        height: 48
                                        opacity: parent.isHovered ? 1.0 : 0.4
                                        Behavior on opacity { NumberAnimation { duration: 200 } }
                                        
                                        Image {
                                            id: similarRightArrowIcon
                                            anchors.centerIn: parent
                                            
                                            source: "qrc:/assets/icons/right_catalog.svg"
                                            
                                            // Large source + mipmap ensures smooth downscaling
                                            sourceSize.width: 128
                                            sourceSize.height: 128
                                            mipmap: true
                                            smooth: true
                                            antialiasing: true
                                            
                                            width: 48
                                            height: 48
                                            fillMode: Image.PreserveAspectFit
                                        }
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
    
    // Stream Selection Dialog
    StreamSelectionDialog {
        id: streamDialog
        parent: root.parent
        
        onStreamSelected: function(stream) {
            console.log("[DetailScreen] Stream selected:", JSON.stringify(stream))
            console.log("[DetailScreen] Stream URL:", stream.url)
            
            // Emit signal to request playback with content data
            if (stream.url) {
                // Get episode info from smartPlayState if available
                var seasonNum = root.smartPlayState.season || root.itemData.seasonNumber || 0
                var episodeNum = root.smartPlayState.episode || root.itemData.episodeNumber || 0
                
                // For TV shows, try to get episode title from current season episodes
                var episodeTitle = ""
                if (root.itemData.type === "tv" && seasonNum > 0 && episodeNum > 0 && root.currentSeasonEpisodes) {
                    for (var i = 0; i < root.currentSeasonEpisodes.length; i++) {
                        var ep = root.currentSeasonEpisodes[i]
                        if (ep.episodeNumber === episodeNum) {
                            episodeTitle = ep.name || ep.title || ""
                            break
                        }
                    }
                }
                
                var contentData = {
                    type: root.itemData.type || "movie",
                    title: root.itemData.type === "tv" && episodeTitle ? episodeTitle : (root.itemData.title || root.itemData.name || ""),
                    description: root.itemData.type === "tv" && episodeTitle ? (root.currentSeasonEpisodes ? (root.currentSeasonEpisodes.find(function(ep) { return ep.episodeNumber === episodeNum }) || {}).overview || "" : "") : (root.itemData.description || root.itemData.overview || ""),
                    logoUrl: root.itemData.logoUrl || root.itemData.logo || "",
                    seasonNumber: seasonNum,
                    episodeNumber: episodeNum
                }
                navigationService.navigateToPlayer(stream.url, contentData)
                root.playRequested(stream.url, contentData)  // Keep for backward compatibility
            }
        }
    }
}