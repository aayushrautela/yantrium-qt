import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import Yantrium.Services 1.0
import "../components"

Item {
    id: root

    // --- Services ---
    property LibraryService libraryService: LibraryService {
        id: libraryService
        Component.onCompleted: console.log("[HomeScreen] LibraryService ready")
    }

    property CatalogPreferencesService catalogPrefsService: CatalogPreferencesService {
        id: catalogPrefsService
    }
    
    property LocalLibraryService localLibraryService: LocalLibraryService {
        id: localLibraryService
    }
    
    property TraktAuthService traktAuthService: TraktAuthService
    
    property TraktWatchlistService traktWatchlistService: TraktWatchlistService {
        id: traktWatchlistService
    }
    
    property StreamService streamService: StreamService {
        id: streamService
    }

    // --- Connections ---
    Connections {
        target: libraryService
        function onCatalogsLoaded(sections) { root.onCatalogsLoaded(sections) }
        function onContinueWatchingLoaded(items) { root.onContinueWatchingLoaded(items) }
        function onError(message) { root.onError(message) }
    }

    // --- Models ---
    ListModel { id: continueWatchingModel }
    ListModel { id: catalogSectionsModel }

    // --- State ---
    property bool isLoading: false
    property var heroItems: []
    property int currentHeroIndex: 0
    
    // --- Signals ---
    signal itemClicked(string contentId, string type, string addonId)

    // --- Hero Logic ---
    function cycleHero(direction) {
        if (heroItems.length <= 1) return
        let newIndex = currentHeroIndex + direction
        if (newIndex >= heroItems.length) newIndex = 0
        if (newIndex < 0) newIndex = heroItems.length - 1
        currentHeroIndex = newIndex
        updateHeroSection(heroItems[currentHeroIndex], true, direction)
        heroRotationTimer.restart()
    }
    
    Timer {
        id: heroRotationTimer
        interval: 10000; running: heroItems.length > 1; repeat: true
        onTriggered: root.cycleHero(1)
    }

    // --- Initialization ---
    Component.onCompleted: {
        console.log("[HomeScreen] Component.onCompleted")
        console.log("[HomeScreen] libraryService:", libraryService)
        // Only load on first creation - MainApp will handle refreshes after watching content
        if (libraryService && catalogSectionsModel.count === 0) {
            console.log("[HomeScreen] Calling loadCatalogs from Component.onCompleted (first load)")
            loadCatalogs()
        } else {
            console.log("[HomeScreen] Skipping initial load - data already exists or service unavailable")
        }
    }

    function loadCatalogs() {
        console.log("[HomeScreen] Requesting catalogs...")
        console.log("[HomeScreen] libraryService exists:", !!libraryService)
        console.log("[HomeScreen] libraryService.isValid:", libraryService ? libraryService.isValid : "N/A")
        isLoading = true
        catalogSectionsModel.clear()
        libraryService.loadCatalogs()
    }

    function onCatalogsLoaded(sections) {
        console.log("[HomeScreen] onCatalogsLoaded called with sections:", sections ? sections.length : "null")
        isLoading = false
        if (!sections || sections.length === 0) {
            console.log("[HomeScreen] No sections received, clearing models")
            catalogSectionsModel.clear()
            return
        }
        console.log("[HomeScreen] Processing", sections.length, "sections")
        catalogSectionsModel.clear()

        // Hero Logic
        heroItems = []
        if (sections[0].items && sections[0].items.length > 0) {
            heroItems = sections[0].items
            currentHeroIndex = 0
            updateHeroSection(heroItems[0], false, 0)
            heroRotationTimer.restart()
        }

        // Populate Models
        for (let i = 0; i < sections.length; i++) {
            let section = sections[i]
            let items = section.items || []
            let cardModel = Qt.createQmlObject('import QtQuick; ListModel {}', catalogSectionsModel)
            
            if (items && items.length > 0) {
                let itemsArray = Array.isArray(items) ? items : []
                if (!Array.isArray(items)) {
                     try { for(let k=0; k<items.length; k++) itemsArray.push(items[k]) } catch(e){}
                }

                for (let j = 0; j < itemsArray.length; j++) {
                    let item = itemsArray[j]
                    cardModel.append({
                        posterUrl: item.posterUrl || item.poster || "",
                        title: item.title || item.name || "Unknown",
                        year: item.year || 0,
                        rating: item.rating || "",
                        progress: item.progress || 0,
                        badgeText: item.badgeText || "",
                        isHighlighted: item.isHighlighted || false,
                        id: item.id || "",
                        type: item.type || section.type || "",
                        addonId: section.addonId || ""
                    })
                }
                console.log("[HomeScreen] Added section:", section.name, "with", cardModel.count, "items")
            } else {
                console.log("[HomeScreen] Section", section.name, "has no items")
            }

            catalogSectionsModel.append({
                sectionTitle: section.name || "Unknown Section",
                addonId: section.addonId || "",
                itemsModel: cardModel 
            })
        }
        console.log("[HomeScreen] Total sections in model:", catalogSectionsModel.count)
    }

    function updateHeroSection(data, animate, direction) {
        if (!heroLoader.item) return
        heroLoader.item.title = data.title || ""
        heroLoader.item.logoUrl = data.logoUrl || data.logo || ""
        heroLoader.item.description = data.description || "No description available."
        
        let meta = []
        if (data.year) meta.push(data.year.toString())
        if (data.rating) meta.push(data.rating.toString())
        heroLoader.item.metadata = meta
        
        let bg = data.background || data.backdrop || ""
        heroLoader.item.updateBackdrop(bg, animate ? (direction || 1) : 0)
    }

    function onContinueWatchingLoaded(items) {
        continueWatchingModel.clear()
        if (!items) return
        for (let i = 0; i < items.length; i++) {
            let item = items[i]
            continueWatchingModel.append({
                backdropUrl: item.backdropUrl || item.backdrop || "",
                logoUrl: item.logoUrl || item.logo || "",
                posterUrl: item.posterUrl || "",
                title: item.title || "",
                type: item.type || "",
                season: item.season || 0,
                episode: item.episode || 0,
                episodeTitle: item.episodeTitle || "",
                progress: item.progress || item.progressPercent || 0,
                rating: item.rating || ""
            })
        }
    }

    function onError(message) {
        console.error("[HomeScreen] Error:", message)
        isLoading = false
        console.log("[HomeScreen] Error state - isLoading:", isLoading, "catalogSectionsModel.count:", catalogSectionsModel.count)
    }
    
    function onHeroPlayClicked() {
        if (heroItems.length === 0 || currentHeroIndex < 0 || currentHeroIndex >= heroItems.length) {
            console.warn("[HomeScreen] No hero item available for play")
            return
        }
        
        var heroItem = heroItems[currentHeroIndex]
        console.log("[HomeScreen] Hero play clicked for:", heroItem.title || heroItem.name)
        
        // Open stream selection dialog
        streamDialog.loadStreams(heroItem, "")
        streamDialog.open()
    }
    
    function onHeroAddToLibraryClicked() {
        if (heroItems.length === 0 || currentHeroIndex < 0 || currentHeroIndex >= heroItems.length) {
            console.warn("[HomeScreen] No hero item available for library")
            return
        }
        
        var heroItem = heroItems[currentHeroIndex]
        console.log("[HomeScreen] Hero add to library clicked for:", heroItem.title || heroItem.name)
        
        // Get content ID (prefer imdbId)
        var contentId = heroItem.imdbId || heroItem.id || ""
        var type = heroItem.type || "movie"
        
        if (!contentId) {
            console.error("[HomeScreen] Cannot add to library: no contentId")
            return
        }
        
        // Ensure we're using imdbId format if available
        if (heroItem.imdbId && heroItem.imdbId.startsWith("tt")) {
            contentId = heroItem.imdbId
        }
        
        // Normalize type
        if (type === "tv" || type === "series") {
            type = "show"
        } else {
            type = "movie"
        }
        
        // Add to library
        if (traktAuthService.isAuthenticated) {
            // Add to Trakt watchlist
            traktWatchlistService.addToWatchlist(type, contentId)
        } else {
            // Add to local library
            var libraryItem = {
                contentId: contentId,
                type: type,
                title: heroItem.title || heroItem.name || "",
                year: heroItem.year || 0,
                posterUrl: heroItem.posterUrl || heroItem.poster || "",
                backdropUrl: heroItem.backdropUrl || heroItem.background || heroItem.backdrop || "",
                description: heroItem.description || "",
                rating: heroItem.rating || heroItem.imdbRating || "",
                tmdbId: heroItem.tmdbId || "",
                imdbId: heroItem.imdbId || ""
            }
            localLibraryService.addToLibrary(libraryItem)
        }
    }

    // --- UI Layout ---
    Rectangle {
        anchors.fill: parent
        color: "#09090b"
        focus: true
        Keys.onRightPressed: cycleHero(1)
        Keys.onLeftPressed: cycleHero(-1)

        Component.onCompleted: {
            console.log("[HomeScreen] UI Rectangle completed, size:", width, "x", height)
        }

        ScrollView {
            anchors.fill: parent
            contentWidth: width
            contentHeight: contentColumn.height
            clip: true

            Component.onCompleted: {
                console.log("[HomeScreen] ScrollView completed, contentHeight:", contentColumn.height)
            }

            Column {
                id: contentColumn
                width: parent.width
                spacing: 40
                bottomPadding: 80
                
                Component.onCompleted: {
                    console.log("[HomeScreen] ContentColumn completed, width:", width)
                }

                // 1. Hero Section (Full width, no margin)
                Loader {
                    id: heroLoader
                    width: parent.width
                    height: 650
                    source: "qrc:/qml/components/HeroSection.qml"
                    
                    onLoaded: {
                        if (item) {
                            item.playClicked.connect(function() {
                                root.onHeroPlayClicked()
                            })
                            item.addToLibraryClicked.connect(function() {
                                root.onHeroAddToLibraryClicked()
                            })
                        }
                    }
                    
                    MouseArea {
                        anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom; width: 100
                        onClicked: root.cycleHero(-1); z: 100
                    }
                    MouseArea {
                        anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: parent.bottom; width: 100
                        onClicked: root.cycleHero(1); z: 100
                    }
                }
                
                // 2. Continue Watching
                Loader {
                    id: cwLoader
                    
                    width: parent.width
                    height: (item && item.implicitHeight > 0) ? item.implicitHeight : (active ? 350 : 0)
                    source: "qrc:/qml/components/HorizontalList.qml"
                    active: continueWatchingModel.count > 0
                    visible: active
                    
                    onLoaded: {
                        if (!item) return
                        item.title = "Continue Watching"
                        item.icon = "" 
                        item.itemWidth = 480 
                        item.itemHeight = 270 
                        item.model = continueWatchingModel
                        // Connect itemClicked signal (for continue watching, we might handle differently)
                        item.itemClicked.connect(function(contentId, type, addonId) {
                            // For continue watching, we can also open detail screen
                            root.itemClicked(contentId, type, addonId)
                        })
                    }
                }

                // 3. Catalog Sections
                Repeater {
                    id: catalogRepeater
                    model: catalogSectionsModel

                    onItemAdded: {
                        console.log("[HomeScreen] Repeater item added, index:", index, "total:", catalogRepeater.count)
                    }

                    Column {
                        id: sectionColumn
                        width: parent.width
                        spacing: 15

                        readonly property string sectionTitle: model.sectionTitle
                        readonly property var sectionItemsModel: model.itemsModel
                        
                        Component.onCompleted: {
                            console.log("[HomeScreen] Section column created:", sectionTitle, "itemsModel:", sectionItemsModel, "items count:", sectionItemsModel ? sectionItemsModel.count : 0)
                        }

                        // Section Title
                        Text {
                            width: parent.width
                            height: 30
                            text: sectionColumn.sectionTitle
                            color: "white"
                            font.pixelSize: 18
                            font.bold: true
                            // Simple Left Padding
                            leftPadding: 50
                            verticalAlignment: Text.AlignVCenter
                        }

                        // Inner ListView container
                        Item {
                            width: parent.width - 100  // Reduce width to leave 50px on each side
                            height: 420 
                            anchors.horizontalCenter: parent.horizontalCenter
                            
                            ListView {
                                id: sectionListView
                                anchors.fill: parent
                                orientation: ListView.Horizontal
                                spacing: 24 
                                leftMargin: 0
                                rightMargin: 0
                                clip: true 
                                model: sectionColumn.sectionItemsModel
                                
                                Component.onCompleted: {
                                    console.log("[HomeScreen] ListView created, model:", sectionColumn.sectionItemsModel, "count:", sectionColumn.sectionItemsModel ? sectionColumn.sectionItemsModel.count : 0)
                                }
                                
                                onCountChanged: {
                                    console.log("[HomeScreen] ListView count changed:", count, "for section:", sectionColumn.sectionTitle)
                                }
                                
                                PropertyAnimation {
                                    id: sectionScrollAnimation
                                    target: sectionListView
                                    property: "contentX"
                                    duration: 300
                                    easing.type: Easing.OutCubic
                                }
                            
                                delegate: Item {
                                    width: 240
                                    height: 400
                                    
                                    property bool isHovered: false
                                    readonly property string sectionAddonId: catalogRepeater.itemAt(index) ? (catalogRepeater.itemAt(index).sectionAddonId || "") : ""

                                    MouseArea {
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onEntered: isHovered = true
                                        onExited: isHovered = false
                                        onClicked: {
                                            var addonId = model.addonId || sectionColumn.sectionAddonId || ""
                                            root.itemClicked(model.id || "", model.type || "", addonId)
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
                                                
                                            Rectangle { id: maskShape; anchors.fill: parent; radius: 8; visible: false }
                                            layer.enabled: true
                                            layer.effect: OpacityMask { maskSource: maskShape }

                                            Rectangle { anchors.fill: parent; color: "#2a2a2a"; radius: 8 }
                                                
                                            Image {
                                                id: img
                                                anchors.fill: parent
                                                source: model.posterUrl || model.poster || ""
                                                fillMode: Image.PreserveAspectCrop
                                                asynchronous: true
                                                smooth: true
                                                scale: isHovered ? 1.10 : 1.0 
                                                Behavior on scale { NumberAnimation { duration: 300; easing.type: Easing.OutQuad } }
                                            }

                                            // Rating
                                            Rectangle {
                                                anchors.bottom: parent.bottom; anchors.right: parent.right; anchors.margins: 8
                                                visible: model.rating !== ""; color: "black"; opacity: 0.8; radius: 4
                                                width: 40; height: 20
                                                Row {
                                                    anchors.centerIn: parent; spacing: 2
                                                    Text { text: "\u2605"; color: "#ffffff"; font.pixelSize: 10 }
                                                    Text { text: model.rating; color: "white"; font.pixelSize: 10; font.bold: true }
                                                }
                                            }

                                            // Border
                                            Rectangle {
                                                anchors.fill: parent; color: "transparent"; radius: 8; z: 10
                                                border.width: 3; border.color: isHovered ? "#ffffff" : "transparent"
                                                Behavior on border.color { ColorAnimation { duration: 200 } }
                                            }
                                        }

                                        // Title
                                        Text {
                                            width: parent.width
                                            text: model.title
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
                                visible: sectionListView.contentX > 0
                                
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
                                        var targetX = Math.max(0, sectionListView.contentX - sectionListView.width * 0.8)
                                        sectionScrollAnimation.to = targetX
                                        sectionScrollAnimation.start()
                                    }
                                }
                            }
                            
                            // Right arrow
                            Item {
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                width: 48
                                height: 48
                                visible: sectionListView.contentX < sectionListView.contentWidth - sectionListView.width
                                
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
                                            sectionListView.contentWidth - sectionListView.width,
                                            sectionListView.contentX + sectionListView.width * 0.8
                                        )
                                        sectionScrollAnimation.to = targetX
                                        sectionScrollAnimation.start()
                                    }
                                }
                            }
                            
                            // Close ListView after arrows
                        }
                    }
                }

                // 4. Loading & Empty States
                Rectangle {
                    width: parent.width; height: 200
                    visible: isLoading
                    color: "transparent"
                    Text { anchors.centerIn: parent; text: "Loading..."; color: "#666"; font.pixelSize: 16 }
                }
                
                Rectangle {
                    width: parent.width; height: 200
                    visible: !isLoading && catalogSectionsModel.count === 0 && continueWatchingModel.count === 0
                    color: "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: "No Content Found\nInstall addons to populate library\n\nDebug: isLoading=" + isLoading + ", sections=" + catalogSectionsModel.count + ", continue=" + continueWatchingModel.count
                        horizontalAlignment: Text.AlignHCenter; color: "#666"; font.pixelSize: 16
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
            console.log("[HomeScreen] Stream selected:", JSON.stringify(stream))
            console.log("[HomeScreen] Stream URL:", stream.url)
            
            // Emit signal to request playback (will be handled by MainApp)
            if (stream.url) {
                root.playRequested(stream.url)
            }
        }
    }
    
    // Signal for playback request
    signal playRequested(string streamUrl)
}