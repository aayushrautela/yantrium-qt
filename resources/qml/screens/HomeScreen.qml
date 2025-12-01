import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Yantrium.Services 1.0

Item {
    id: root

    // Services
    property LibraryService libraryService: LibraryService {
        id: libraryService
        
        Component.onCompleted: {
            console.log("[HomeScreen] LibraryService component created")
        }
        
        onCatalogsLoaded: function(sections) {
            console.log("[HomeScreen] LibraryService.catalogsLoaded signal received directly")
        }
        
        onError: function(message) {
            console.error("[HomeScreen] LibraryService error signal:", message)
        }
    }
    
    property CatalogPreferencesService catalogPrefsService: CatalogPreferencesService {
        id: catalogPrefsService
        
        Component.onCompleted: {
            console.log("[HomeScreen] CatalogPreferencesService component created")
        }
    }

    // Connect to LibraryService signals automatically
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

    // Models for data
    ListModel {
        id: continueWatchingModel
    }

    // Store catalog sections as JS array (QML ListModel can't handle complex nested objects)
    property var catalogSections: []

    // State
    property bool isLoading: false

    // --- MERGED COMPONENT.ONCOMPLETED ---
    // This is the single entry point for initialization
    Component.onCompleted: {
        console.log("[HomeScreen] ===== Component.onCompleted =====")
        console.log("[HomeScreen] Component completed")
        console.log("[HomeScreen] libraryService:", libraryService)
        
        if (!libraryService) {
            console.error("[HomeScreen] ERROR: libraryService is null!")
            return
        }
        
        console.log("[HomeScreen] Calling loadCatalogs()")
        loadCatalogs()
    }

    function loadCatalogs() {
        console.log("[HomeScreen] ===== loadCatalogs() called =====")
        isLoading = true
        catalogSections = []
        console.log("[HomeScreen] Cleared catalog sections array, calling libraryService.loadCatalogs()")
        libraryService.loadCatalogs()
    }

    // Signal handlers
    function onCatalogsLoaded(sections) {
        console.log("[HomeScreen] ===== onCatalogsLoaded() called =====")
        console.log("[HomeScreen] Sections received:", sections ? sections.length : 0)
        isLoading = false

        if (!sections) {
            console.log("[HomeScreen] ERROR: sections is null or undefined!")
            catalogSections = []
            return
        }

        if (sections.length === 0) {
            console.log("[HomeScreen] WARNING: sections array is empty!")
            catalogSections = []
        }

        // Store sections directly in JS array (QML ListModel can't handle nested objects)
        catalogSections = sections

        console.log("[HomeScreen] ✓ Stored", catalogSections.length, "sections in JS array")

        // Debug: Log each section
        for (let i = 0; i < catalogSections.length; i++) {
            let section = catalogSections[i]
            console.log("[HomeScreen] Section", i, "- name:", section.name, "items:", section.items ? section.items.length : 0)
        }

        // Force Repeater update by resetting model
        catalogRepeater.model = 0
        catalogRepeater.model = catalogSections.length
    }

    function onContinueWatchingLoaded(items) {
        console.log("[HomeScreen] Continue watching loaded, items:", items ? items.length : 0)

        continueWatchingModel.clear()

        if (!items) return

        for (let i = 0; i < items.length; i++) {
            let item = items[i]
            continueWatchingModel.append({
                posterUrl: item.posterUrl || "",
                title: item.title || "",
                year: item.year || 0,
                rating: item.rating || "",
                progress: item.progressPercent || item.progress || 0,
                progressPercent: item.progressPercent || item.progress || 0,
                badgeText: (item.progress > 0) ? "UP NEXT" : "",
                isHighlighted: false,
                type: item.type || "",
                imdbId: item.imdbId || "",
                season: item.season || 0,
                episode: item.episode || 0
            })
        }
    }

    function onError(message) {
        console.error("[HomeScreen] Error:", message)
        isLoading = false
    }

    Rectangle {
        id: backgroundRect
        anchors.fill: parent
        color: "#09090b"

        // Main content area
        ScrollView {
            anchors.fill: parent
            contentWidth: width
            contentHeight: contentColumn.height
            clip: true 

            Column {
                id: contentColumn
                width: parent.width
                spacing: 20
                
                // Hero section (featured content)
                Loader {
                    id: heroLoader
                    width: parent.width
                    height: 500
                    source: "qrc:/qml/components/HeroSection.qml"
                    active: hasHeroCatalogs && heroItemsModel.count > 0
                    visible: active
                    
                    property bool hasHeroCatalogs: false
                    property int currentHeroIndex: 0
                    
                    function setCurrentHeroIndex(index) {
                        currentHeroIndex = index
                    }
                    
                    ListModel {
                        id: heroItemsModel
                    }
                    
                    Component.onCompleted: {
                        checkHeroCatalogs()
                        loadHeroItems()
                    }
                    
                    function checkHeroCatalogs() {
                        let heroCatalogs = catalogPrefsService.getHeroCatalogs()
                        hasHeroCatalogs = heroCatalogs.length > 0
                        console.log("[HomeScreen] Hero catalogs:", heroCatalogs.length)
                    }
                    
                    function loadHeroItems() {
                        if (!hasHeroCatalogs) return
                        console.log("[HomeScreen] Loading hero items...")
                        libraryService.loadHeroItems()
                    }
                    
                    function updateHeroDisplay() {
                        if (!item || heroItemsModel.count === 0) return
                        
                        let heroItem = heroItemsModel.get(heroLoader.currentHeroIndex)
                        if (!heroItem) return
                        
                        item.title = heroItem.title || heroItem.name || ""
                        item.description = heroItem.description || ""
                        
                        // Build metadata array
                        let meta = []
                        if (heroItem.year) meta.push(heroItem.year.toString())
                        if (heroItem.rating) meta.push(heroItem.rating.toString())
                        if (heroItem.genres && heroItem.genres.length > 0) {
                            meta.push(heroItem.genres[0])
                        }
                        item.metadata = meta
                        
                        // Set images
                        item.backdropUrl = heroItem.background || heroItem.backdropUrl || heroItem.poster || ""
                        item.posterUrl = heroItem.poster || heroItem.posterUrl || ""
                        
                        // Enable navigation if multiple items
                        item.hasMultipleItems = heroItemsModel.count > 1
                    }
                    
                    onLoaded: {
                        if (!item) return
                        updateHeroDisplay()
                    }
                    
                    Connections {
                        target: libraryService
                        ignoreUnknownSignals: true
                        
                        function onHeroItemsLoaded(items) {
                            console.log("[HomeScreen] Hero items loaded:", items ? items.length : 0)
                            heroItemsModel.clear()
                            
                            if (!items || items.length === 0) {
                                hasHeroCatalogs = false
                                return
                            }
                            
                            // Limit to 10 items
                            let itemsToAdd = items.slice(0, 10)
                            for (let i = 0; i < itemsToAdd.length; i++) {
                                let item = itemsToAdd[i]
                                heroItemsModel.append({
                                    title: item.title || item.name || "",
                                    name: item.name || item.title || "",
                                    description: item.description || "",
                                    background: item.background || item.backdropUrl || "",
                                    poster: item.poster || item.posterUrl || "",
                                    year: item.year || 0,
                                    rating: item.rating || "",
                                    genres: item.genres || [],
                                    id: item.id || "",
                                    type: item.type || "",
                                    imdbId: item.imdbId || ""
                                })
                            }
                            
                            heroLoader.setCurrentHeroIndex(0)
                            heroLoader.updateHeroDisplay()
                        }
                    }
                    
                    Connections {
                        target: heroLoader.item
                        ignoreUnknownSignals: true 
                        
                        function onPlayClicked() {
                            let heroItem = heroItemsModel.get(heroLoader.currentHeroIndex)
                            if (heroItem) {
                                console.log("[HomeScreen] Play clicked for:", heroItem.title)
                                // TODO: Navigate to player
                            }
                        }
                        
                        function onAddToLibraryClicked() {
                            let heroItem = heroItemsModel.get(heroLoader.currentHeroIndex)
                            if (heroItem) {
                                console.log("[HomeScreen] Add to library clicked for:", heroItem.title)
                                // TODO: Add to library
                            }
                        }
                        
                        function onPreviousClicked() {
                            let newIndex = heroLoader.currentHeroIndex > 0 
                                ? heroLoader.currentHeroIndex - 1 
                                : heroItemsModel.count - 1
                            heroLoader.setCurrentHeroIndex(newIndex)
                            heroLoader.updateHeroDisplay()
                        }
                        
                        function onNextClicked() {
                            let newIndex = heroLoader.currentHeroIndex < heroItemsModel.count - 1
                                ? heroLoader.currentHeroIndex + 1
                                : 0
                            heroLoader.setCurrentHeroIndex(newIndex)
                            heroLoader.updateHeroDisplay()
                        }
                    }
                    
                    Connections {
                        target: catalogPrefsService
                        ignoreUnknownSignals: true
                        
                        function onCatalogsUpdated() {
                            heroLoader.checkHeroCatalogs()
                            heroLoader.loadHeroItems()
                        }
                    }
                }
                
                // Continue Watching section
                Loader {
                    id: cwLoader
                    width: parent.width
                    height: item ? item.implicitHeight : (active ? 320 : 0)
                    
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
                
                // Catalog sections - SIMPLE DIRECT APPROACH
                
                // Catalog sections - CARDS DISPLAY (JS array storage, dynamic models)
                Repeater {
                    id: catalogRepeater
                    model: catalogSections.length

                    Column {
                        width: parent.width
                        spacing: 10

                        // Section title
                        Text {
                            width: parent.width
                            height: 40
                            text: (catalogSections[index].name || "Unknown") + " (" + (catalogSections[index].items ? catalogSections[index].items.length : 0) + " items)"
                            color: "white"
                            font.pixelSize: 18
                            font.bold: true
                            leftPadding: 20
                            verticalAlignment: Text.AlignVCenter
                        }

                        // Horizontal scrolling list of cards
                        ListView {
                            id: sectionListView
                            width: parent.width
                            height: 250
                            orientation: ListView.Horizontal
                            spacing: 12
                            leftMargin: 20
                            rightMargin: 20

                            Component.onCompleted: {
                                // Create model dynamically from JS array (processed items)
                                let section = catalogSections[index]
                                if (!section || !section.items) {
                                    console.log("[HomeScreen] No section data or items for index:", index)
                                    return
                                }

                                let sectionModel = Qt.createQmlObject('import QtQuick; ListModel {}', sectionListView)
                                let items = section.items

                                for (let i = 0; i < items.length; i++) {
                                    let item = items[i]
                                    sectionModel.append({
                                        title: item.title || "",
                                        poster: item.poster || item.posterUrl || "",
                                        background: item.background || item.backdropUrl || "",
                                        year: item.year || 0,
                                        rating: item.rating || "",
                                        id: item.id || ""
                                    })
                                }

                                sectionListView.model = sectionModel
                                console.log("[HomeScreen] ✓ Created section", section.name, "with", sectionModel.count, "items")
                            }

                            delegate: Rectangle {
                                width: 150
                                height: 225
                                color: "#1a1a1a"
                                radius: 8

                                Column {
                                    anchors.fill: parent
                                    spacing: 8

                                    // Poster image (processed data has resolved URLs)
                                    Rectangle {
                                        width: parent.width
                                        height: 200
                                        color: "#2a2a2a"
                                        radius: 4

                                        Image {
                                            anchors.fill: parent
                                            source: model.poster || ""
                                            fillMode: Image.PreserveAspectCrop
                                            asynchronous: true

                                            onStatusChanged: {
                                                if (status === Image.Error) {
                                                    console.log("[HomeScreen] Image failed to load:", source)
                                                }
                                            }
                                        }
                                    }

                                    // Title (processed data uses 'title' field)
                                    Text {
                                        width: parent.width
                                        height: 25
                                        text: model.title || ""
                                        color: "white"
                                        font.pixelSize: 12
                                        elide: Text.ElideRight
                                        leftPadding: 4
                                        rightPadding: 4
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        console.log("Clicked on:", model.title)
                                    }
                                }
                            }
                        }
                    }
                }
                
                // Loading indicator
                Rectangle {
                    width: parent.width
                    height: 200
                    color: "transparent"
                    visible: isLoading

                    Text {
                        anchors.centerIn: parent
                        text: "Loading catalogs..."
                        font.pixelSize: 16
                        color: "#aaaaaa"
                    }
                }

                // Empty state
                Rectangle {
                    width: parent.width
                    height: 200
                    color: "transparent"
                    visible: !isLoading && catalogSections.length === 0 && continueWatchingModel.count === 0

                    Column {
                        anchors.centerIn: parent
                        spacing: 12

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "No content available"
                            font.pixelSize: 18
                            color: "#aaaaaa"
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "Install addons to see catalogs"
                            font.pixelSize: 14
                            color: "#666666"
                        }
                    }
                }
            }
        }
    }
}