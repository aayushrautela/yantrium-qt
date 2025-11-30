import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Yantrium.Services 1.0

Item {
    id: root

    // Services
    property LibraryService libraryService: LibraryService {
        id: libraryService
    }
    
    property CatalogPreferencesService catalogPrefsService: CatalogPreferencesService {
        id: catalogPrefsService
    }

    // Connect to LibraryService signals automatically
    // This replaces the manual .connect() logic and prevents duplicate connections
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

    ListModel {
        id: catalogSectionsModel
    }

    // State
    property bool isLoading: false

    // Load data on component completion
    Component.onCompleted: {
        console.log("[HomeScreen] Component completed, loading catalogs")
        loadCatalogs()
    }

    function loadCatalogs() {
        isLoading = true
        // No need to manually connect signals here anymore due to the Connections object above
        libraryService.loadCatalogs()
    }

    // Signal handlers
    function onCatalogsLoaded(sections) {
        console.log("[HomeScreen] Catalogs loaded, sections:", sections ? sections.length : 0)
        isLoading = false

        catalogSectionsModel.clear()

        if (!sections) return

        for (let i = 0; i < sections.length; i++) {
            let section = sections[i]
            let items = section.items || []
            console.log("[HomeScreen] Adding section:", section.name, "with", items.length, "items")
            
            catalogSectionsModel.append({
                name: section.name || "",
                type: section.type || "",
                addonId: section.addonId || "",
                items: items // We store the array here to parse it later in the Loader
            })
        }

        console.log("[HomeScreen] Total catalog sections:", catalogSectionsModel.count)
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
            clip: true // Good practice to keep content from drawing outside scrollview

            Column {
                id: contentColumn
                width: parent.width
                spacing: 20
                
                // Hero section (featured content) - only show if hero catalogs are configured
                Loader {
                    id: heroLoader
                    width: parent.width
                    height: 500
                    source: "qrc:/qml/components/HeroSection.qml"
                    active: hasHeroCatalogs
                    visible: active
                    
                    property bool hasHeroCatalogs: false
                    
                    Component.onCompleted: {
                        checkHeroCatalogs()
                    }
                    
                    function checkHeroCatalogs() {
                        let heroCatalogs = catalogPrefsService.getHeroCatalogs()
                        hasHeroCatalogs = heroCatalogs.length > 0
                    }
                    
                    onLoaded: {
                        if (!item) return
                        // For now, use placeholder data
                        // TODO: Load actual hero items from hero catalogs
                        item.title = "Zootopia"
                        item.description = "After cracking the biggest case in Zootopia's history, rookie cops Judy Hopps and Nick Wilde find themselves on the twisting trail of a great mystery when Gary De'Snake arrives and turns the animal metropolis upside down."
                        item.metadata = ["26-11-2025", "PG", "Animation", "Family"]
                        item.hasMultipleItems = false
                    }
                    
                    // Use Connections for Loader items to ensure clean signal handling
                    Connections {
                        target: heroLoader.item
                        ignoreUnknownSignals: true 
                        
                        function onPlayClicked() {
                            console.log("Play clicked")
                        }
                        
                        function onAddToLibraryClicked() {
                            console.log("Add to library clicked")
                        }
                    }
                    
                    Connections {
                        target: catalogPrefsService
                        ignoreUnknownSignals: true
                        
                        function onCatalogsUpdated() {
                            heroLoader.checkHeroCatalogs()
                        }
                    }
                }
                
                // Continue Watching section
                Loader {
                    width: parent.width
                    source: "qrc:/qml/components/HorizontalList.qml"
                    active: continueWatchingModel.count > 0 // Only load if there is data
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
                
                // Catalog sections (dynamically created)
                Repeater {
                    id: catalogRepeater
                    model: catalogSectionsModel
                    
                    Loader {
                        width: parent.width
                        source: "qrc:/qml/components/HorizontalList.qml"
                        
                        // Capture the model data for this specific row
                        property var sectionData: modelData
                        
                        onLoaded: {
                            if (!item) return
                            item.title = sectionData.name || ""
                            item.icon = ""
                            item.itemWidth = 150
                            item.itemHeight = 225
                            
                            // Create ListModel from items array dynamically
                            // Updated import to generic QtQuick to avoid version conflicts
                            let sectionItems = sectionData.items || []
                            let sectionItemsModel = Qt.createQmlObject('import QtQuick; ListModel {}', item)
                            
                            for (let i = 0; i < sectionItems.length; i++) {
                                sectionItemsModel.append(sectionItems[i])
                            }
                            
                            item.model = sectionItemsModel
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
                    visible: !isLoading && catalogSectionsModel.count === 0 && continueWatchingModel.count === 0

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