import QtQuick
import QtQuick.Controls
import Qt5Compat.GraphicalEffects
import Yantrium.Services 1.0

Item {
    id: root
    
    property LibraryService libraryService: LibraryService {
        id: libraryService
        Component.onCompleted: {
            console.warn("[SearchScreen] ===== LibraryService CREATED =====")
            console.warn("[SearchScreen] LibraryService object:", libraryService)
            console.warn("[SearchScreen] LibraryService.searchTmdb:", typeof libraryService.searchTmdb)
        }
    }
    
    signal itemClicked(string contentId, string type, string addonId)
    
    property bool isLoading: false
    property string currentQuery: ""
    property string searchQuery: ""
    property string selectedFilter: "All"  // All, Movies, TV Shows, People
    
    Component.onCompleted: {
        console.warn("[SearchScreen] ===== SearchScreen COMPONENT LOADED =====")
        console.warn("[SearchScreen] libraryService:", libraryService)
    }
    
    Rectangle {
        anchors.fill: parent
        color: "#09090b"
        
        Column {
            anchors.fill: parent
            spacing: 0
            
            // Header section with title and filters
            Column {
                id: headerSection
                width: parent.width
                anchors.left: parent.left
                anchors.leftMargin: 50
                anchors.right: parent.right
                anchors.rightMargin: 50
                spacing: 20
                topPadding: 40
                bottomPadding: 30
                visible: currentQuery.length > 0
                
                // Results heading
                Row {
                    spacing: 8
                    
                    Text {
                        text: "Results for "
                        font.pixelSize: 32
                        font.bold: true
                        color: "#ffffff"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    
                    Text {
                        text: "\"" + currentQuery + "\""
                        font.pixelSize: 32
                        font.bold: true
                        color: "#3b82f6"  // Blue highlight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                
                // Filter buttons
                Row {
                    spacing: 12
                    
                    Repeater {
                        model: ["All", "Movies", "TV Shows", "People"]
                        
                        Rectangle {
                            width: filterText.width + 24
                            height: 36
                            radius: 4
                            color: root.selectedFilter === modelData ? "#ffffff" : "#2d2d2d"
                            
                            Text {
                                id: filterText
                                anchors.centerIn: parent
                                text: modelData
                                font.pixelSize: 14
                                font.bold: root.selectedFilter === modelData
                                color: root.selectedFilter === modelData ? "#000000" : "#ffffff"
                            }
                            
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    root.selectedFilter = modelData
                                    applyFilter()
                                }
                            }
                        }
                    }
                }
            }
            
            // Results area
            Item {
                width: parent.width
                height: currentQuery.length > 0 ? parent.height - headerSection.implicitHeight : parent.height
                
                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 50
                    clip: true
                    
                    GridView {
                        id: searchGrid
                        width: parent.width
                        height: parent.height
                        model: searchModel
                    cellWidth: 252  // 240 + 12 spacing (matches LibraryScreen catalog cards)
                    cellHeight: 412  // 400 + 12 spacing (matches LibraryScreen catalog cards)
                    
                    delegate: Item {
                        width: 240  // Match LibraryScreen catalog card width
                        height: 400  // Match LibraryScreen catalog card height

                        property bool isHovered: false

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: isHovered = true
                            onExited: isHovered = false
                            onClicked: {
                                // Note: Search results from TMDB only have TMDB ID
                                // MediaMetadataService will convert TMDB to IMDB when loading details
                                var contentId = model.contentId || model.tmdbId || ""
                                var type = model.type || ""
                                root.itemClicked(contentId, type, "")
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
                                    source: model.posterUrl || ""
                                    fillMode: Image.PreserveAspectCrop
                                    asynchronous: true
                                    smooth: true
                                    scale: isHovered ? 1.10 : 1.0
                                    Behavior on scale { NumberAnimation { duration: 300; easing.type: Easing.OutQuad } }
                                }

                                // Rating overlay (exactly like LibraryScreen)
                                Rectangle {
                                    anchors.bottom: parent.bottom; anchors.right: parent.right; anchors.margins: 8
                                    visible: model.rating !== "" && model.rating !== "0.0"
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
                                return "Searching..."
                            } else if (currentQuery.length > 0 && searchModel.count === 0) {
                                if (root.selectedFilter !== "All") {
                                    return "No " + root.selectedFilter.toLowerCase() + " found for \"" + currentQuery + "\""
                                } else {
                                    return "No results found for \"" + currentQuery + "\""
                                }
                            } else {
                                return "Enter a search query to find movies and TV shows"
                            }
                        }
                        font.pixelSize: 24
                        color: "#aaaaaa"
                        horizontalAlignment: Text.AlignHCenter
                        visible: searchModel.count === 0
                    }
                }
            }
            }
        }
    }
    
    ListModel {
        id: searchModel
    }
    
    property var allResults: []  // Store all results for filtering
    
    Connections {
        target: libraryService
        
        function onTmdbSearchResultsLoaded(results) {
            console.warn("[SearchScreen] ===== onTmdbSearchResultsLoaded CALLED =====")
            console.warn("[SearchScreen] Received" + results.length + "search results")
            
            root.isLoading = false
            root.allResults = results  // Store all results
            applyFilter()  // Apply current filter
        }
        
        function onError(message) {
            console.error("[SearchScreen] ===== ERROR RECEIVED =====")
            console.error("[SearchScreen] Error message:", message)
            root.isLoading = false
        }
    }
    
    function applyFilter() {
        searchModel.clear()
        
        if (root.allResults.length === 0) {
            return
        }
        
        var filtered = root.allResults
        
        // Apply filter based on selectedFilter
        if (root.selectedFilter === "Movies") {
            filtered = filtered.filter(function(item) {
                return (item.type || "").toLowerCase() === "movie"
            })
        } else if (root.selectedFilter === "TV Shows") {
            filtered = filtered.filter(function(item) {
                var type = (item.type || "").toLowerCase()
                return type === "tv" || type === "series" || type === "show"
            })
        } else if (root.selectedFilter === "People") {
            filtered = filtered.filter(function(item) {
                return (item.type || "").toLowerCase() === "person"
            })
        }
        // "All" shows everything, no filtering needed
        
        // Add filtered results to model
        for (var i = 0; i < filtered.length; i++) {
            var item = filtered[i]
            var cardData = {
                contentId: item.contentId || item.tmdbId || "",
                tmdbId: item.tmdbId || "",
                title: item.title || item.name || "",
                year: item.year || 0,
                posterUrl: item.posterUrl || "",
                rating: item.rating || "",
                type: item.type || ""
            }
            searchModel.append(cardData)
        }
        
        console.warn("[SearchScreen] Filtered results:", searchModel.count, "items for filter:", root.selectedFilter)
    }
    
    function performSearch() {
        console.warn("[SearchScreen] ===== performSearch CALLED =====")
        var query = searchQuery || ""
        console.warn("[SearchScreen] Query:", query)
        
        if (query.length === 0) {
            console.warn("[SearchScreen] Empty query, returning")
            return
        }
        
        root.isLoading = true
        root.currentQuery = query
        searchModel.clear()
        root.allResults = []
        
        console.warn("[SearchScreen] Calling libraryService.searchTmdb with query:", query)
        console.warn("[SearchScreen] libraryService object:", libraryService)
        console.warn("[SearchScreen] libraryService.searchTmdb function:", typeof libraryService.searchTmdb)
        
        if (typeof libraryService.searchTmdb === 'function') {
            libraryService.searchTmdb(query)
            console.warn("[SearchScreen] searchTmdb called successfully")
        } else {
            console.error("[SearchScreen] ERROR: searchTmdb is not a function!")
        }
        
        // Clear searchQuery after using it
        searchQuery = ""
    }
    
    onSearchQueryChanged: {
        if (searchQuery && searchQuery.length > 0) {
            performSearch()
        }
    }
}

