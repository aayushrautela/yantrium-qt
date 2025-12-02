import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Yantrium.Services 1.0

Item {
    id: root

    // --- Services ---
    property LibraryService libraryService: LibraryService {
        id: libraryService
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

    ListModel {
        id: catalogSectionsModel
    }

    // --- State ---
    property bool isLoading: false

    // --- Initialization ---
    Component.onCompleted: {
        console.log("[HomeScreen] Component initialized")
        if (libraryService) {
            loadCatalogs()
        } else {
            console.error("[HomeScreen] LibraryService is missing")
        }
    }

    function loadCatalogs() {
        console.log("[HomeScreen] Requesting catalogs...")
        isLoading = true
        catalogSectionsModel.clear()
        libraryService.loadCatalogs()
    }

    // --- Signal Handlers ---
    function onCatalogsLoaded(sections) {
        console.log("[HomeScreen] Sections loaded:", sections ? sections.length : 0)
        isLoading = false
        catalogSectionsModel.clear()

        if (!sections || sections.length === 0) return

        // 1. Hero Logic
        if (sections[0].items && sections[0].items.length > 0) {
            updateHeroSection(sections[0].items[0])
        }

        // 2. Populate Catalog Model
        for (let i = 0; i < sections.length; i++) {
            let section = sections[i]
            
            // Note: We use 'sectionTitle' as the role name
            catalogSectionsModel.append({
                sectionTitle: section.name || "",
                addonId: section.addonId || "",
                items: section.items || [] 
            })
        }
        console.log("[HomeScreen] Model populated. Count:", catalogSectionsModel.count)
    }

    function updateHeroSection(data) {
        if (!heroLoader.item) return
        heroLoader.item.title = data.title || ""
        heroLoader.item.description = data.description || "No description available."
        
        let meta = []
        if (data.year) meta.push(data.year.toString())
        if (data.rating) meta.push(data.rating.toString())
        heroLoader.item.metadata = meta
        
        heroLoader.item.backgroundImage = data.background || data.poster || ""
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

                // 3. Dynamic Catalogs
                Repeater {
                    id: catalogRepeater
                    model: catalogSectionsModel

                    delegate: Item {
                        id: sectionWrapper
                        width: parent.width
                        // FIX: Break the loop. Wrapper takes height of Loader, but Loader has a default.
                        height: sectionLoader.height

                        // Visual Debugging: Blue box behind every row
                        Rectangle {
                            anchors.fill: parent
                            color: "blue"
                            opacity: 0.1
                            z: 0
                        }

                        // Capture Data
                        readonly property string myTitle: sectionTitle 
                        readonly property var myItems: items

                        Loader {
                            id: sectionLoader
                            width: parent.width 
                            // FIX: Force a minimum height so it is never 0
                            height: (item && item.implicitHeight > 0) ? item.implicitHeight : 320
                            z: 1

                            source: "qrc:/qml/components/HorizontalList.qml"

                            onLoaded: {
                                console.log("[Repeater] Loader finished for:", sectionWrapper.myTitle)
                                
                                if (!item) return

                                // Properties
                                item.title = sectionWrapper.myTitle
                                item.icon = ""
                                item.itemWidth = 150
                                item.itemHeight = 225
                                
                                // Explicitly set height on the loaded item
                                item.height = 320 

                                // Data Conversion
                                let sourceItems = sectionWrapper.myItems
                                let newList = Qt.createQmlObject('import QtQuick; ListModel {}', item)
                                
                                let count = 0
                                let isList = false
                                
                                if (Array.isArray(sourceItems)) {
                                    count = sourceItems.length
                                } else if (sourceItems && typeof sourceItems.count === 'number') {
                                    count = sourceItems.count
                                    isList = true
                                }
                                
                                console.log("[Repeater] Adding", count, "items to", sectionWrapper.myTitle)

                                for (let i = 0; i < count; i++) {
                                    let entry = isList ? sourceItems.get(i) : sourceItems[i]
                                    newList.append(entry)
                                }

                                item.model = newList
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
                    }
                }
                
                Rectangle {
                    width: parent.width
                    height: 200
                    visible: !isLoading && catalogSectionsModel.count === 0 && continueWatchingModel.count === 0
                    color: "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: "No Content Found"
                        color: "#666"
                    }
                }
            }
        }
    }
}



