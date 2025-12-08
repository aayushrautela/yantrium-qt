import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import Yantrium.Services 1.0

Item {
    id: root
    
    // TraktAuthService is a singleton, accessed directly
    property TraktAuthService traktAuthService: TraktAuthService
    
    // TraktCoreService is a singleton, accessed directly
    property TraktCoreService traktService: TraktCoreService
    
    // All services are singletons, accessed directly
    property LocalLibraryService localLibrary: LocalLibraryService
    property LibraryService libraryService: LibraryService
    property MediaMetadataService metadataService: MediaMetadataService
    property NavigationService navigationService: NavigationService
    property LoggingService loggingService: LoggingService
    
    signal itemClicked(string contentId, string type, string addonId)
    
    property bool isLoading: false
    property var libraryItems: []
    property var pendingTraktItems: ({})  // Map of contentId -> Trakt item data
    property int pendingMetadataRequests: 0
    
    ListModel {
        id: libraryModel
    }
    
    function mapTraktWatchlistItem(item) {
        // Trakt watchlist items have structure: { movie: {...} } or { show: {...} }
        // Handle QVariantMap from C++ - use bracket notation for property access
        var movie = item.movie || item["movie"] || {}
        var show = item.show || item["show"] || {}
        
        // Check which one exists - handle both dot and bracket notation
        var hasMovie = (movie && (movie.title || movie["title"])) || false
        var hasShow = (show && (show.title || show["title"])) || false
        
        var content = hasMovie ? movie : (hasShow ? show : {})
        
        // Extract nested properties - handle both notations
        var ids = content.ids || content["ids"] || {}
        var images = content.images || content["images"] || {}
        var poster = images.poster || images["poster"] || {}
        var backdrop = images.backdrop || images["backdrop"] || {}
        var logo = images.logo || images["logo"] || {}
        
        // Get image URLs - Trakt uses "full" for full resolution
        var posterUrl = poster.full || poster["full"] || ""
        var backdropUrl = backdrop.full || backdrop["backdrop"] || ""
        var logoUrl = logo.full || logo["full"] || ""
        
        var type = hasMovie ? "movie" : "series"
        var imdbId = ids.imdb || ids["imdb"] || ""
        var tmdbId = ids.tmdb || ids["tmdb"] || 0
        var contentId = imdbId || (tmdbId ? ("tmdb:" + tmdbId) : "")
        
        var title = content.title || content["title"] || ""
        var year = content.year || content["year"] || 0
        
        // Removed debug log - unnecessary
        
        return {
            contentId: contentId,
            type: type,
            title: title,
            year: year,
            posterUrl: posterUrl,
            backdropUrl: backdropUrl,
            logoUrl: logoUrl,
            rating: "",
            imdbId: imdbId,
            tmdbId: tmdbId ? String(tmdbId) : ""
        }
    }
    
    function loadLibrary() {
        console.log("[LibraryScreen] loadLibrary called, authenticated:", traktAuthService.isAuthenticated, "current model count:", libraryModel.count)
        isLoading = true
        libraryModel.clear()
        pendingTraktItems = {}
        pendingMetadataRequests = 0

        if (traktAuthService.isAuthenticated) {
            // Load Trakt watchlist
            console.log("[LibraryScreen] Loading Trakt watchlist")
            traktService.getWatchlistMoviesWithImages()
            traktService.getWatchlistShowsWithImages()
        } else {
            // Load local library
            console.log("[LibraryScreen] Loading local library")
            localLibrary.getLibraryItems()
        }
    }
    
    function enrichTraktItemWithTmdb(traktItem) {
        // Extract contentId and type from Trakt item
        var movie = traktItem.movie || traktItem["movie"] || {}
        var show = traktItem.show || traktItem["show"] || {}
        var hasMovie = (movie && (movie.title || movie["title"])) || false
        var content = hasMovie ? movie : (show || {})
        var ids = content.ids || content["ids"] || {}
        var imdbId = ids.imdb || ids["imdb"] || ""
        var tmdbId = ids.tmdb || ids["tmdb"] || 0
        var contentId = imdbId || (tmdbId ? ("tmdb:" + tmdbId) : "")
        var type = hasMovie ? "movie" : "series"
        
        if (!contentId) {
            console.warn("[LibraryScreen] Cannot enrich item: no contentId", JSON.stringify(traktItem))
            return
        }
        
        // Store Trakt item for later merging
        pendingTraktItems[contentId] = traktItem
        pendingMetadataRequests++
        
        console.log("[LibraryScreen] Requesting TMDB metadata for:", contentId, "type:", type)
        metadataService.getCompleteMetadata(contentId, type)
    }
    
    function mergeTraktAndTmdbData(traktItem, tmdbData) {
        // Extract basic info from Trakt
        var movie = traktItem.movie || traktItem["movie"] || {}
        var show = traktItem.show || traktItem["show"] || {}
        var hasMovie = (movie && (movie.title || movie["title"])) || false
        var content = hasMovie ? movie : (show || {})
        var ids = content.ids || content["ids"] || {}
        var imdbId = ids.imdb || ids["imdb"] || ""
        var tmdbId = ids.tmdb || ids["tmdb"] || 0
        var contentId = imdbId || (tmdbId ? ("tmdb:" + tmdbId) : "")
        var type = hasMovie ? "movie" : "series"
        
        // Use TMDB data for display (better images, ratings, etc.)
        // But keep Trakt IDs for identification
        var enriched = {
            contentId: contentId,
            type: type,
            title: tmdbData.title || tmdbData["title"] || content.title || content["title"] || "",
            year: tmdbData.year || tmdbData["year"] || content.year || content["year"] || 0,
            posterUrl: tmdbData.posterUrl || tmdbData["posterUrl"] || "",
            backdropUrl: tmdbData.backdropUrl || tmdbData["backdropUrl"] || "",
            logoUrl: tmdbData.logoUrl || tmdbData["logoUrl"] || "",
            rating: tmdbData.rating || tmdbData["rating"] || tmdbData.imdbRating || tmdbData["imdbRating"] || "",
            imdbId: imdbId,
            tmdbId: tmdbData.tmdbId || tmdbData["tmdbId"] || (tmdbId ? String(tmdbId) : "")
        }
        
        console.log("[LibraryScreen] Merged Trakt + TMDB data:", enriched.title, "posterUrl:", enriched.posterUrl)
        return enriched
    }
    
    function checkIfAllMetadataLoaded() {
        if (pendingMetadataRequests <= 0) {
            console.log("[LibraryScreen] All metadata requests completed")
            isLoading = false
        }
    }
    
    Connections {
        target: traktService
        function onWatchlistMoviesFetched(movies) {
            console.log("[LibraryScreen] Trakt watchlist movies fetched:", movies.length)
            if (movies.length === 0) {
                // Check if we're done (no shows pending either)
                checkIfAllMetadataLoaded()
                return
            }
            
            // Enrich each movie with TMDB data
            for (var i = 0; i < movies.length; i++) {
                var item = movies[i]
                var itemObj = (typeof item === 'object' && item !== null) ? item : {}
                enrichTraktItemWithTmdb(itemObj)
            }
        }
        function onWatchlistShowsFetched(shows) {
            console.log("[LibraryScreen] Trakt watchlist shows fetched:", shows.length)
            if (shows.length === 0) {
                // Check if we're done
                checkIfAllMetadataLoaded()
                return
            }
            
            // Enrich each show with TMDB data
            for (var i = 0; i < shows.length; i++) {
                var item = shows[i]
                var itemObj = (typeof item === 'object' && item !== null) ? item : {}
                enrichTraktItemWithTmdb(itemObj)
            }
        }
        function onError(message) {
            console.error("[LibraryScreen] Trakt error:", message)
            isLoading = false
        }
    }
    
    Connections {
        target: metadataService
        function onMetadataLoaded(metadata) {
            console.log("[LibraryScreen] TMDB metadata loaded for:", metadata.title || metadata["title"])
            
            // Find the corresponding Trakt item
            var contentId = metadata.id || metadata["id"] || metadata.imdbId || metadata["imdbId"] || ""
            if (!contentId) {
                console.warn("[LibraryScreen] Metadata loaded but no contentId found")
                pendingMetadataRequests--
                checkIfAllMetadataLoaded()
                return
            }
            
            var traktItem = pendingTraktItems[contentId]
            if (!traktItem) {
                // Try with IMDB ID format
                var imdbId = metadata.imdbId || metadata["imdbId"] || ""
                if (imdbId) {
                    traktItem = pendingTraktItems[imdbId]
                }
            }
            
            if (!traktItem) {
                console.warn("[LibraryScreen] No Trakt item found for contentId:", contentId)
                pendingMetadataRequests--
                checkIfAllMetadataLoaded()
                return
            }
            
            // Merge Trakt + TMDB data
            var enriched = mergeTraktAndTmdbData(traktItem, metadata)
            
            // Add to model
            libraryModel.append(enriched)
            console.log("[LibraryScreen] Added enriched item to library, count:", libraryModel.count)
            
            // Clean up
            delete pendingTraktItems[contentId]
            if (metadata.imdbId || metadata["imdbId"]) {
                delete pendingTraktItems[metadata.imdbId || metadata["imdbId"]]
            }
            
            pendingMetadataRequests--
            checkIfAllMetadataLoaded()
        }
        function onError(message) {
            console.error("[LibraryScreen] MediaMetadataService error:", message)
            pendingMetadataRequests--
            checkIfAllMetadataLoaded()
        }
    }
    
    Connections {
        target: localLibrary
        function onLibraryItemsLoaded(items) {
            console.log("[LibraryScreen] Local library items loaded:", items.length, "items")
            libraryModel.clear()
            for (var i = 0; i < items.length; i++) {
                var item = items[i]
                console.log("[LibraryScreen] Item", i, ":", JSON.stringify(item))
                console.log("[LibraryScreen] Adding item:", item.title, "contentId:", item.contentId, "posterUrl:", item.posterUrl)
                // Convert QVariantMap to plain object for ListModel
                libraryModel.append({
                    contentId: item.contentId || "",
                    type: item.type || "",
                    title: item.title || "",
                    year: item.year || 0,
                    posterUrl: item.posterUrl || "",
                    backdropUrl: item.backdropUrl || "",
                    logoUrl: item.logoUrl || "",
                    rating: item.rating || "",
                    imdbId: item.imdbId || "",
                    tmdbId: item.tmdbId || ""
                })
            }
            console.log("[LibraryScreen] Library model count after loading:", libraryModel.count)
            isLoading = false
        }
        function onLibraryItemAdded(success) {
            console.log("[LibraryScreen] Library item added, success:", success, "- refresh will be handled by MainApp")
            // Refresh is now handled by MainApp via libraryChanged signal
        }
        function onError(message) {
            console.error("[LibraryScreen] Local library error:", message)
            isLoading = false
        }
    }
    
    Connections {
        target: traktAuthService
        function onAuthenticationStatusChanged(authenticated) {
            loadLibrary()
        }
    }
    
    Component.onCompleted: {
        loadLibrary()
    }
    
    Rectangle {
        anchors.fill: parent
        color: "#09090b"
        
        ScrollView {
            anchors.fill: parent
            anchors.margins: 50
            clip: true
            
            GridView {
                id: libraryGrid
                width: parent.width
                height: parent.height
                model: libraryModel
                cellWidth: 252  // 240 + 12 spacing (matches HomeScreen catalog cards)
                cellHeight: 412  // 400 + 12 spacing (matches HomeScreen catalog cards)
                
                Component.onCompleted: {
                    console.log("[LibraryScreen] GridView completed, model count:", libraryModel.count)
                }
                
                onModelChanged: {
                    console.log("[LibraryScreen] GridView model changed, count:", libraryModel.count)
                }
                
                onCountChanged: {
                    console.log("[LibraryScreen] GridView count changed:", count)
                }
                
                delegate: Item {
                    width: 240  // Match HomeScreen catalog card width
                    height: 400  // Match HomeScreen catalog card height

                    property bool isHovered: false

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: isHovered = true
                        onExited: isHovered = false
                        onClicked: {
                            var contentId = model.contentId || ""
                            var type = model.type || ""
                            navigationService.navigateToDetail(contentId, type, "")
                            root.itemClicked(contentId, type, "")  // Keep for backward compatibility
                        }
                        cursorShape: Qt.PointingHandCursor
                    }

                    Column {
                        anchors.fill: parent
                        spacing: 12

                        // Poster (exactly like HomeScreen catalog cards)
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
                                source: model.posterUrl || ""
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                                smooth: true
                                scale: isHovered ? 1.10 : 1.0
                                Behavior on scale { NumberAnimation { duration: 300; easing.type: Easing.OutQuad } }
                            }

                            // Rating overlay (exactly like HomeScreen)
                            Rectangle {
                                anchors.bottom: parent.bottom; anchors.right: parent.right; anchors.margins: 8
                                visible: model.rating !== ""
                                color: "black"
                                opacity: 0.8
                                radius: 4
                                width: 40
                                height: 20
                                Row {
                                    anchors.centerIn: parent; spacing: 2
                                    Text { text: "\u2605"; color: "#ffffff"; font.pixelSize: 10 }
                                    Text { text: model.rating; color: "white"; font.pixelSize: 10; font.bold: true }
                                }
                            }

                            // Border (exactly like HomeScreen)
                            Rectangle {
                                anchors.fill: parent; color: "transparent"; radius: 8; z: 10
                                border.width: 3; border.color: isHovered ? "#ffffff" : "transparent"
                                Behavior on border.color { ColorAnimation { duration: 200 } }
                            }
                        }

                        // Title (exactly like HomeScreen - centered below poster)
                        Text {
                            width: parent.width
                            text: model.title || ""
                            color: "white"
                            font.pixelSize: 14
                            font.bold: true
                            elide: Text.ElideRight
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }
                
                // Empty state
                Text {
                    anchors.centerIn: parent
                    text: {
                        if (isLoading) {
                            return "Loading..."
                        } else if (traktAuthService.isAuthenticated) {
                            return "No items in watchlist"
                        } else {
                            return "No items in library\n\nLog in to Trakt to sync your watchlist"
                        }
                    }
                    font.pixelSize: 24
                    color: "#aaaaaa"
                    horizontalAlignment: Text.AlignHCenter
                    visible: libraryModel.count === 0 && !isLoading
                }
                
                // Loading state
                Text {
                    anchors.centerIn: parent
                    text: "Loading..."
                    font.pixelSize: 24
                    color: "#aaaaaa"
                    horizontalAlignment: Text.AlignHCenter
                    visible: isLoading && libraryModel.count === 0
                }
            }
        }
    }
}

