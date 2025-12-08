import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import Yantrium.Services 1.0

Item {
    id: root

    property LibraryService libraryService: LibraryService
    property NavigationService navigationService: NavigationService
    property LoggingService loggingService: LoggingService
    signal itemClicked(string contentId, string type, string addonId)
    signal closeRequested()  // Signal to go back to previous screen

    property bool isLoading: false
    property string currentQuery: ""
    property string searchQuery: ""
    property string selectedFilter: "All"  // Filter by catalog type: "All", "Movies", "TV Shows", "Anime Series", "Anime Movies"

    // Store sections (like HomeScreen data flow)
    ListModel { 
        id: searchSectionsModel 
    }

    // Combined flat model for GridView (like LibraryScreen)
    ListModel {
        id: combinedResultsModel
    }

    // Map section names to filter types
    function getFilterType(sectionName) {
        if (sectionName === "Movies") return "Movies"
        if (sectionName === "TV Shows") return "TV Shows"
        if (sectionName === "Anime Series") return "Anime Series"
        if (sectionName === "Anime Movies") return "Anime Movies"
        return "All"
    }

    // Update combined model from all sections
    function updateCombinedModel() {
        combinedResultsModel.clear()
        
        for (var i = 0; i < searchSectionsModel.count; i++) {
            var section = searchSectionsModel.get(i)
            var sectionTitle = section.sectionTitle || ""
            var itemsModel = section.itemsModel
            
            if (!itemsModel) continue
            
            var filterType = getFilterType(sectionTitle)
            
            // Apply filter
            if (selectedFilter !== "All" && selectedFilter !== filterType) {
                continue
            }
            
            // Limit items per catalog when "All" is selected (max 15 per catalog)
            var maxItems = (selectedFilter === "All") ? 15 : itemsModel.count
            
            // Add items from this section (limited if "All" filter)
            for (var j = 0; j < Math.min(itemsModel.count, maxItems); j++) {
                var item = itemsModel.get(j)
                combinedResultsModel.append({
                    posterUrl: item.posterUrl || item.poster || "",
                    title: item.title || item.name || "Unknown",
                    year: item.year || 0,
                    rating: item.rating || item.imdbRating || "",
                    id: item.id || item.imdbId || item.tmdbId || "",
                    imdbId: item.imdbId || "",
                    tmdbId: item.tmdbId || "",
                    type: item.type || "",
                    addonId: item.addonId || section.addonId || "",
                    filterType: filterType
                })
            }
        }
    }

    Component.onCompleted: {
        // Removed debug log - unnecessary
    }

    Rectangle {
        anchors.fill: parent
        color: "#09090b"

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // Header Section with Query
            Column {
                Layout.fillWidth: true
                Layout.topMargin: 40
                Layout.bottomMargin: 20
                Layout.leftMargin: 50
                Layout.rightMargin: 50
                spacing: 20
                visible: currentQuery.length > 0

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
                        color: "#3b82f6"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                // Filter Buttons (like catalog filtering)
                Row {
                    spacing: 12
                    visible: searchSectionsModel.count > 0

                    Repeater {
                        model: ["All", "Movies", "TV Shows", "Anime Series", "Anime Movies"]
                        delegate: Rectangle {
                            width: filterText.width + 24
                            height: 36
                            radius: 18
                            color: selectedFilter === modelData ? "#3b82f6" : "#1a1a1a"
                            border.width: 1
                            border.color: selectedFilter === modelData ? "#3b82f6" : "#333333"

                            property bool isHovered: false

                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                                onEntered: parent.isHovered = true
                                onExited: parent.isHovered = false
                                onClicked: {
                                    selectedFilter = modelData
                                    updateCombinedModel()
                                }
                                cursorShape: Qt.PointingHandCursor
                            }

                            Text {
                                id: filterText
                                anchors.centerIn: parent
                                text: modelData
                                color: selectedFilter === modelData ? "#ffffff" : "#aaaaaa"
                                font.pixelSize: 14
                                font.bold: selectedFilter === modelData
                            }
                        }
                    }
                }
            }

            // Results Grid (like LibraryScreen)
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                Item {
                    width: parent.width - 100  // Subtract left and right margins
                    height: parent.height
                    anchors.horizontalCenter: parent.horizontalCenter

                    GridView {
                        id: resultsGrid
                        anchors.fill: parent
                        model: combinedResultsModel
                        cellWidth: 252  // 240 + 12 spacing (matches HomeScreen catalog cards)
                        cellHeight: 412  // 400 + 12 spacing (matches HomeScreen catalog cards)

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
                                var contentId = model.id || model.imdbId || model.tmdbId || ""
                                var type = model.type || ""
                                var addonId = model.addonId || ""
                                navigationService.navigateToDetail(contentId, type, addonId || "")
                                navigationService.navigateToDetail(contentId, type, addonId || "")
                                root.itemClicked(contentId, type, addonId)  // Keep for backward compatibility  // Keep for backward compatibility
                            }
                            cursorShape: Qt.PointingHandCursor
                        }

                        Column {
                            anchors.fill: parent
                            spacing: 12

                            // Poster (exactly like HomeScreen/LibraryScreen catalog cards)
                            Item {
                                id: imageContainer
                                width: parent.width
                                height: 360

                                Rectangle { id: maskShape; anchors.fill: parent; radius: 8; visible: false }
                                layer.enabled: true
                                layer.effect: OpacityMask { maskSource: maskShape }

                                Rectangle { anchors.fill: parent; color: "#2a2a2a"; radius: 8 }

                                Image {
                                    anchors.fill: parent
                                    source: model.posterUrl || model.poster || ""
                                    fillMode: Image.PreserveAspectCrop
                                    asynchronous: true
                                    smooth: true
                                    scale: isHovered ? 1.10 : 1.0
                                    Behavior on scale { NumberAnimation { duration: 300; easing.type: Easing.OutQuad } }
                                }

                                // Rating overlay (exactly like HomeScreen)
                                Rectangle {
                                    anchors.bottom: parent.bottom
                                    anchors.right: parent.right
                                    anchors.margins: 8
                                    visible: (model.rating || "") !== "" && (model.rating || "") !== "0.0"
                                    color: "black"
                                    opacity: 0.8
                                    radius: 4
                                    width: 40
                                    height: 20
                                    Row {
                                        anchors.centerIn: parent
                                        spacing: 2
                                        Text { text: "\u2605"; color: "#ffffff"; font.pixelSize: 10 }
                                        Text {
                                            text: model.rating || ""
                                            color: "white"
                                            font.pixelSize: 10
                                            font.bold: true
                                        }
                                    }
                                }

                                // Border (exactly like HomeScreen)
                                Rectangle {
                                    anchors.fill: parent
                                    color: "transparent"
                                    radius: 8
                                    z: 10
                                    border.width: 3
                                    border.color: isHovered ? "#ffffff" : "transparent"
                                    Behavior on border.color { ColorAnimation { duration: 200 } }
                                }
                            }

                            // Title (exactly like HomeScreen - centered below poster)
                            Text {
                                width: parent.width
                                text: model.title || model.name || "Unknown"
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
                            } else if (currentQuery.length > 0 && combinedResultsModel.count === 0) {
                                if (selectedFilter !== "All") {
                                    return "No " + selectedFilter + " found for \"" + currentQuery + "\""
                                }
                                return "No results found for \"" + currentQuery + "\""
                            }
                            return "Enter a search query"
                        }
                        font.pixelSize: 24
                        color: "#aaaaaa"
                        horizontalAlignment: Text.AlignHCenter
                        visible: combinedResultsModel.count === 0
                    }

                    // Loading state
                    Text {
                        anchors.centerIn: parent
                        text: "Searching..."
                        font.pixelSize: 24
                        color: "#aaaaaa"
                        horizontalAlignment: Text.AlignHCenter
                        visible: isLoading && combinedResultsModel.count === 0
                    }
                }
            }
        }
        }
    }

    Connections {
        target: libraryService
        function onSearchSectionLoaded(section) {
            root.isLoading = false
            onSearchSectionReceived(section)
        }
        function onError(message) {
            console.error("[SearchScreen] Error:", message)
            root.isLoading = false
        }
    }

    function onSearchSectionReceived(section) {
        if (!section || !section.name) return

        var items = section.items || []
        
        // Prevent duplicates in the visual list
        for (var i = 0; i < searchSectionsModel.count; i++) {
            if (searchSectionsModel.get(i).sectionTitle === section.name) {
                return
            }
        }

        // Create ListModel using the same pattern as HomeScreen
        var cardModel = Qt.createQmlObject('import QtQuick; ListModel {}', searchSectionsModel)

        // Handle both array and array-like objects from C++
        var itemsArray = Array.isArray(items) ? items : []
        if (!Array.isArray(items)) {
            try { for (var k = 0; k < items.length; k++) itemsArray.push(items[k]) } catch(e){}
        }

        // Populate the ListModel with items
        for (var j = 0; j < itemsArray.length; j++) {
            var item = itemsArray[j]
            cardModel.append({
                posterUrl: item.posterUrl || item.poster || "",
                title: item.title || item.name || "Unknown",
                year: item.year || 0,
                rating: item.rating || item.imdbRating || "",
                id: item.id || item.imdbId || item.tmdbId || "",
                imdbId: item.imdbId || "",
                tmdbId: item.tmdbId || "",
                type: item.type || section.type || "",
                addonId: section.addonId || ""
            })
        }

        // Store the ListModel in searchSectionsModel (same as HomeScreen)
        searchSectionsModel.append({
            sectionTitle: section.name || "Unknown Section",
            addonId: section.addonId || "",
            itemsModel: cardModel
        })

        // Update combined model after adding new section
        updateCombinedModel()
    }

    function performSearch() {
        var query = searchQuery || ""
        if (query.length === 0) return

        if (root.currentQuery !== query) {
            searchSectionsModel.clear()
            combinedResultsModel.clear()
            selectedFilter = "All"  // Reset filter on new search
        }

        root.isLoading = true
        root.currentQuery = query

        // Use searchCatalogs instead (addon-based search)
        libraryService.searchCatalogs(query)
        searchQuery = ""
    }

    onSearchQueryChanged: {
        if (searchQuery && searchQuery.length > 0) {
            performSearch()
        }
    }
}
