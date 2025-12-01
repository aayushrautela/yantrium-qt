import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
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

        // 2. Hero Logic (Pick first item of first section)
        if (sections[0].items && sections[0].items.length > 0) {
            updateHeroSection(sections[0].items[0])
        }

        // 3. Populate Outer ListModel with explicit data mapping
        // Create inner ListModel for each section immediately
        for (let i = 0; i < sections.length; i++) {
            let section = sections[i]
            let items = section.items || []
            
            console.log("[HomeScreen] Section", i, "- name:", section.name, "items type:", typeof items, "isArray:", Array.isArray(items), "length:", items ? items.length : 0)

            // Create inner ListModel for cards right away
            let cardModel = Qt.createQmlObject('import QtQuick; ListModel {}', catalogSectionsModel)
            
            // EXPLICIT DATA MAPPING: Convert items to card model
            if (items && items.length > 0) {
                // Handle both JS arrays and QVariantList from C++
                let itemsArray = []
                if (Array.isArray(items)) {
                    itemsArray = items
                } else {
                    // Convert QVariantList to array
                    try {
                        for (let j = 0; j < items.length; j++) {
                            itemsArray.push(items[j])
                        }
                    } catch (e) {
                        console.error("[HomeScreen] Error converting items array:", e)
                    }
                }
                
                // Debug first item structure
                if (itemsArray.length > 0) {
                    console.log("[HomeScreen] First item in section", i, "keys:", Object.keys(itemsArray[0]))
                }
                
                // EXPLICIT MAPPING: Convert raw items to clean card objects
                for (let j = 0; j < itemsArray.length; j++) {
                    let item = itemsArray[j]
                    
                    // Extract values explicitly - handle QVariantMap access
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
                
                console.log("[HomeScreen] ✓ Created", cardModel.count, "cards for section:", section.name)
            }

            // Store section with its card model
            catalogSectionsModel.append({
                sectionTitle: section.name || "Unknown Section",
                addonId: section.addonId || "",
                itemsModel: cardModel // Store the pre-created ListModel
            })
        }

        console.log("[HomeScreen] ✓ Populated outer ListModel with", catalogSectionsModel.count, "sections")
    }

    function updateHeroSection(data) {
        if (!heroLoader.item) return
        heroLoader.item.title = data.title || ""
        heroLoader.item.description = data.description || "No description available."
        
        let meta = []
        if (data.year) meta.push(data.year.toString())
        if (data.rating) meta.push(data.rating.toString())
        heroLoader.item.metadata = meta
        
        heroLoader.item.backdropUrl = data.background || data.poster || ""
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
                bottomPadding: 50 // Extra space at bottom

                // 1. Hero Section
                Loader {
                    id: heroLoader
                    width: parent.width
                    height: 500
                    source: "qrc:/qml/components/HeroSection.qml"
                }

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

                // 3. Catalog Sections (Nested ListModel Approach)
                Repeater {
                    id: catalogRepeater
                    model: catalogSectionsModel

                    // Section Container
                    Column {
                        id: sectionColumn
                        width: parent.width
                        spacing: 10

                        // Capture model data as properties
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
                            leftPadding: 20
                            verticalAlignment: Text.AlignVCenter
                        }

                        // Inner ListView with pre-created ListModel
                        ListView {
                            id: sectionListView
                            width: parent.width
                            height: 380
                            orientation: ListView.Horizontal
                            spacing: 20
                            leftMargin: 20
                            rightMargin: 20
                            clip: true
                            model: sectionColumn.sectionItemsModel
                            
                            // Debug model changes
                            onCountChanged: {
                                console.log("[HomeScreen] ListView count changed to:", count, "for section:", sectionColumn.sectionTitle)
                            }
                            
                            Component.onCompleted: {
                                console.log("[HomeScreen] ListView created for section:", sectionColumn.sectionTitle)
                                console.log("[HomeScreen] Model:", sectionColumn.sectionItemsModel)
                                console.log("[HomeScreen] Model count:", sectionColumn.sectionItemsModel ? sectionColumn.sectionItemsModel.count : 0)
                            }

                            delegate: Item {
                                width: 240
                                height: 360
                                
                                property bool isHovered: false

                                Rectangle {
                                    id: contentRect
                                    anchors.fill: parent
                                    color: "transparent"
                                    radius: 8
                                    
                                    Column {
                                        anchors.fill: parent
                                        spacing: 12

                                        // Poster Wrapper (Clipped)
                                        Rectangle {
                                            width: parent.width
                                            height: parent.height - 40
                                            color: "#2a2a2a"
                                            radius: 8
                                            clip: true

                                            Image {
                                                id: img
                                                anchors.fill: parent
                                                source: model.posterUrl || model.poster || ""
                                                fillMode: Image.PreserveAspectCrop
                                                asynchronous: true
                                                smooth: true
                                                
                                                scale: isHovered ? 1.1 : 1.0
                                                Behavior on scale { NumberAnimation { duration: 300 } }
                                            }
                                            
                                            Text {
                                                anchors.centerIn: parent
                                                text: img.status === Image.Error ? "No Image" : ""
                                                color: "#666"
                                                visible: img.status !== Image.Ready
                                            }

                                            // Rating Badge
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

                                        // Title Text
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

                                MouseArea {
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onEntered: isHovered = true
                                    onExited: isHovered = false
                                    onClicked: {
                                        console.log("Clicked catalog item:", model.title, "ID:", model.id)
                                        // TODO: Open item details modal
                                    }
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