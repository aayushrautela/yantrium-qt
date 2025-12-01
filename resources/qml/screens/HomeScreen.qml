import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import Yantrium.Services 1.0

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

    // --- Connections ---
    Connections {
        target: libraryService
        
        function onCatalogsLoaded(sections) {
            root.onCatalogsLoaded(sections)
        }

        function onContinueWatchingLoaded(items) {
            root.onContinueWatchingLoaded(items)
        }

        function onError(message) {
            root.onError(message)
        }
    }

    // --- Models ---
    ListModel {
        id: continueWatchingModel
    }

    // Outer ListModel: Stores section metadata + raw items array
    ListModel {
        id: catalogSectionsModel
    }

    // --- State ---
    property bool isLoading: false
    property var heroItems: []
    property int currentHeroIndex: 0
    
    // Timer for auto-rotating hero
    Timer {
        id: heroRotationTimer
        interval: 10000 // 10 seconds
        running: heroItems.length > 1
        repeat: true
        onTriggered: {
            currentHeroIndex = (currentHeroIndex + 1) % heroItems.length
            updateHeroSection(heroItems[currentHeroIndex], true)
        }
    }

    // --- Initialization ---
    Component.onCompleted: {
        if (libraryService) {
            loadCatalogs()
        }
    }

    function loadCatalogs() {
        console.log("[HomeScreen] Requesting catalogs...")
        isLoading = true
        catalogSectionsModel.clear()
        libraryService.loadCatalogs()
    }

    // --- Logic ---
    function onCatalogsLoaded(sections) {
        console.log("[HomeScreen] Sections loaded:", sections ? sections.length : 0)
        isLoading = false

        if (!sections || sections.length === 0) {
            catalogSectionsModel.clear()
            return
        }

        // 1. Clear existing data
        catalogSectionsModel.clear()

        // 2. Hero Logic (Store all items from first section for rotation)
        heroItems = []
        if (sections[0].items && sections[0].items.length > 0) {
            heroItems = sections[0].items
            currentHeroIndex = 0
            updateHeroSection(heroItems[0], false)
            heroRotationTimer.restart()
        }

        // 3. Populate Outer ListModel with explicit data mapping
        for (let i = 0; i < sections.length; i++) {
            let section = sections[i]
            let items = section.items || []
            
            // Create inner ListModel for cards right away
            let cardModel = Qt.createQmlObject('import QtQuick; ListModel {}', catalogSectionsModel)
            
            // EXPLICIT DATA MAPPING: Convert items to card model
            if (items && items.length > 0) {
                let itemsArray = []
                if (Array.isArray(items)) {
                    itemsArray = items
                } else {
                    try {
                        for (let j = 0; j < items.length; j++) {
                            itemsArray.push(items[j])
                        }
                    } catch (e) {
                        console.error("[HomeScreen] Error converting items array:", e)
                    }
                }
                
                for (let j = 0; j < itemsArray.length; j++) {
                    let item = itemsArray[j]
                    
                    let posterUrl = item.posterUrl || item.poster || ""
                    let title = item.title || item.name || "Unknown"
                    let year = 0
                    if (item.year !== undefined) {
                        year = typeof item.year === 'number' ? item.year : parseInt(item.year) || 0
                    }
                    let rating = item.rating || ""
                    let progress = item.progress || 0
                    let badgeText = item.badgeText || ""
                    let isHighlighted = item.isHighlighted || false
                    let id = item.id || ""
                    
                    cardModel.append({
                        posterUrl: posterUrl,
                        title: title,
                        year: year,
                        rating: rating,
                        progress: progress,
                        badgeText: badgeText,
                        isHighlighted: isHighlighted,
                        id: id
                    })
                }
            }

            catalogSectionsModel.append({
                sectionTitle: section.name || "Unknown Section",
                addonId: section.addonId || "",
                itemsModel: cardModel 
            })
        }
    }

    function updateHeroSection(data, animate) {
        if (!heroLoader.item) return
        
        // Update text content instantly (no animation)
        heroLoader.item.title = data.title || ""
        heroLoader.item.logoUrl = data.logoUrl || data.logo || ""
        heroLoader.item.description = data.description || "No description available."
        
        let meta = []
        if (data.year) meta.push(data.year.toString())
        if (data.rating) meta.push(data.rating.toString())
        heroLoader.item.metadata = meta
        
        // Update backdrop with animation if requested
        heroLoader.item.updateBackdrop(data.background || "", animate || false)
    }

    function onContinueWatchingLoaded(items) {
        continueWatchingModel.clear()
        if (!items) return

        for (let i = 0; i < items.length; i++) {
            let item = items[i]
            continueWatchingModel.append({
                posterUrl: item.posterUrl || "",
                title: item.title || "",
                year: item.year || 0,
                rating: item.rating || "",
                progress: item.progress || 0,
                badgeText: (item.progress > 0) ? "UP NEXT" : "",
                imdbId: item.imdbId || ""
            })
        }
    }

    function onError(message) {
        console.error("[HomeScreen] Error:", message)
        isLoading = false
    }

    // --- UI Layout ---
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
                spacing: 20
                bottomPadding: 50

                // 1. Hero Section (full width, independent of margins)
                Loader {
                    id: heroLoader
                    width: parent.width
                    height: 650
                    source: "qrc:/qml/components/HeroSection.qml"
                }
                
                // Content wrapper with margins for everything except hero
                Column {
                    width: parent.width
                    spacing: 20
                    leftPadding: 40
                    rightPadding: 40

                    // 2. Continue Watching
                    Loader {
                        id: cwLoader
                        width: parent.width
                        height: (item && item.implicitHeight > 0) ? item.implicitHeight : (active ? 320 : 0)
                        
                        source: "qrc:/qml/components/HorizontalList.qml"
                        active: continueWatchingModel.count > 0
                        visible: active
                        
                        onLoaded: {
                            if (!item) return
                            item.title = "Continue Watching"
                            item.icon = "\U0001f550"
                            item.itemWidth = 150
                            item.itemHeight = 225
                            item.model = continueWatchingModel
                        }
                    }

                    // 3. Catalog Sections
                Repeater {
                    id: catalogRepeater
                    model: catalogSectionsModel

                    Column {
                        id: sectionColumn
                        width: parent.width
                        spacing: 10

                        readonly property string sectionTitle: model.sectionTitle
                        readonly property var sectionItemsModel: model.itemsModel

                        // Section Title
                        Text {
                            width: parent.width
                            height: 40
                            text: sectionColumn.sectionTitle
                            color: "white"
                            font.pixelSize: 18
                            font.bold: true
                            leftPadding: 0
                            verticalAlignment: Text.AlignVCenter
                        }

                        // Inner ListView
                        ListView {
                            id: sectionListView
                            width: parent.width
                            height: 380 // Total height for delegate
                            orientation: ListView.Horizontal
                            spacing: 20
                            leftMargin: 0
                            rightMargin: 0
                            clip: true
                            model: sectionColumn.sectionItemsModel
                            
                            delegate: Item {
                                width: 240
                                height: 360
                                
                                property bool isHovered: false

                                // Mouse handling for the entire card
                                MouseArea {
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onEntered: isHovered = true
                                    onExited: isHovered = false
                                    onClicked: {
                                        console.log("Clicked catalog item:", model.title, "ID:", model.id)
                                    }
                                }

                                Rectangle {
                                    id: contentRect
                                    anchors.fill: parent
                                    color: "transparent"
                                    radius: 8
                                    
                                    Column {
                                        anchors.fill: parent
                                        spacing: 12

                                        // --- FIX: Image Wrapper with Scale Animation ---
                                        Item {
                                            width: parent.width
                                            height: parent.height - 40 // Leave space for text
                                            
                                            // 1. Define the mask shape (invisible, used by layer)
                                            Rectangle {
                                                id: maskShape
                                                anchors.fill: parent
                                                radius: 8
                                                visible: false
                                            }

                                            // 2. Enable Layering and set the OpacityMask
                                            layer.enabled: true
                                            layer.effect: OpacityMask {
                                                maskSource: maskShape
                                            }

                                            // Background placeholder
                                            Rectangle {
                                                anchors.fill: parent
                                                color: "#2a2a2a"
                                            }
                                            
                                            // 3. The Image itself
                                            Image {
                                                id: img
                                                anchors.fill: parent
                                                source: model.posterUrl || model.poster || ""
                                                fillMode: Image.PreserveAspectCrop
                                                asynchronous: true
                                                smooth: true
                                                
                                                // 4. Scale Logic: Zoom from 1.0 to 1.15
                                                scale: isHovered ? 1.15 : 1.0
                                                
                                                Behavior on scale { 
                                                    NumberAnimation { duration: 300; easing.type: Easing.OutQuad } 
                                                }
                                            }

                                            // Loading/Error state
                                            Text {
                                                anchors.centerIn: parent
                                                text: img.status === Image.Error ? "No Image" : ""
                                                color: "#666"
                                                visible: img.status !== Image.Ready
                                            }

                                            // Rating Badge (Inside the masked area)
                                            Rectangle {
                                                anchors.bottom: parent.bottom
                                                anchors.right: parent.right
                                                anchors.margins: 8
                                                visible: model.rating && model.rating !== ""
                                                color: "black"
                                                opacity: 0.8
                                                radius: 4
                                                width: 40
                                                height: 20
                                                
                                                Row {
                                                    anchors.centerIn: parent
                                                    spacing: 2
                                                    Text { text: "\u2605"; color: "#bb86fc"; font.pixelSize: 10 }
                                                    Text { text: model.rating; color: "white"; font.pixelSize: 10; font.bold: true }
                                                }
                                            }
                                        }

                                        // Title Text (Outside the masked area)
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

                                // Border overlay
                                Rectangle {
                                    anchors.fill: parent
                                    color: "transparent"
                                    radius: 8
                                    z: 10
                                    
                                    border.width: 3
                                    border.color: isHovered ? "#bb86fc" : "transparent"
                                    
                                    Behavior on border.color { ColorAnimation { duration: 200 } }
                                }
                            }
                        }
                    }
                    }

                    // 4. Loading & Empty States
                    Rectangle {
                        width: parent.width
                        height: 200
                        visible: isLoading
                        color: "transparent"
                        Text {
                            anchors.centerIn: parent
                            text: "Loading..."
                            color: "#666"
                            font.pixelSize: 16
                        }
                    }
                    
                    Rectangle {
                        width: parent.width
                        height: 200
                        visible: !isLoading && catalogSectionsModel.count === 0 && continueWatchingModel.count === 0
                        color: "transparent"
                        Text {
                            anchors.centerIn: parent
                            text: "No Content Found\nInstall addons to populate library"
                            horizontalAlignment: Text.AlignHCenter
                            color: "#666"
                            font.pixelSize: 16
                        }
                    }
                }
            }
        }
    }
}