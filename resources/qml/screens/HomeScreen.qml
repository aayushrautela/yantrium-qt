import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import Yantrium.Services 1.0
import "../components"

Item {
    id: root

    property LibraryService libraryService: LibraryService
    property CatalogPreferencesService catalogPrefsService: CatalogPreferencesService
    property LocalLibraryService localLibraryService: LocalLibraryService
    property TraktAuthService traktAuthService: TraktAuthService
    property StreamService streamService: StreamService
    property TraktWatchlistService traktWatchlistService: TraktWatchlistService
    property NavigationService navigationService: NavigationService
    property LoggingService loggingService: LoggingService

    Connections {
        target: libraryService
        function onCatalogsLoaded(sections) { root.onCatalogsLoaded(sections) }
        function onContinueWatchingLoaded(items) { root.onContinueWatchingLoaded(items) }
        function onError(message) { root.onError(message) }
    }

    ListModel { id: continueWatchingModel }
    ListModel { id: catalogSectionsModel }

    property bool isLoading: false
    property var heroItems: []
    property int currentHeroIndex: 0
    
    // Navigation is now handled via NavigationService - signals kept for backward compatibility
    signal itemClicked(string contentId, string type, string addonId)

    // Cycle hero carousel, wrapping around at ends
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

    Component.onCompleted: {
        console.log("[HomeScreen] Component.onCompleted")
        // Load only on first creation; MainApp handles refreshes after playback
        if (libraryService && catalogSectionsModel.count === 0) {
            loadCatalogs()
        }
    }

    function loadCatalogs() {
        isLoading = true
        catalogSectionsModel.clear()
        libraryService.loadCatalogs()
    }

    function onCatalogsLoaded(sections) {
        isLoading = false
        if (!sections || sections.length === 0) {
            catalogSectionsModel.clear()
            return
        }
        catalogSectionsModel.clear()

        // Setup Hero section from the first catalog item
        heroItems = []
        if (sections[0].items && sections[0].items.length > 0) {
            heroItems = sections[0].items
            currentHeroIndex = 0
            updateHeroSection(heroItems[0], false, 0)
            heroRotationTimer.restart()
        }

        // Populate catalog rows
        for (let i = 0; i < sections.length; i++) {
            let section = sections[i]
            let items = section.items || []
            let cardModel = Qt.createQmlObject('import QtQuick; ListModel {}', catalogSectionsModel)
            
            // Handle both array and array-like objects from C++
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
                    imdbId: item.imdbId || "",
                    tmdbId: item.tmdbId || "",
                    type: item.type || section.type || "",
                    addonId: section.addonId || ""
                })
            }

            catalogSectionsModel.append({
                sectionTitle: section.name || "Unknown Section",
                addonId: section.addonId || "",
                itemsModel: cardModel 
            })
        }
    }

    function updateHeroSection(data, animate, direction) {
        if (!heroLoader.item) return

        heroLoader.item.title = data.title || ""
        heroLoader.item.logoUrl = data.logoUrl || data.logo || ""
        heroLoader.item.description = data.description || "No description available."

        let meta = []
        if (data.rating) meta.push(data.rating.toString())
        if (data.year) meta.push(data.year.toString())
        if (data.runtimeFormatted) meta.push(data.runtimeFormatted)

        // Robust genre handling for different data types (Array, QVariantList, String)
        let genreList = []
        if (data.genres) {
            if (Array.isArray(data.genres)) {
                genreList = data.genres
            } else if (typeof data.genres === 'object' && data.genres.length !== undefined) {
                for (let i = 0; i < data.genres.length; i++) genreList.push(data.genres[i])
            } else if (typeof data.genres === 'string' && data.genres.includes(',')) {
                genreList = data.genres.split(',')
            }
        }

        if (genreList.length > 0) {
            let displayGenres = genreList.slice(0, 2).map(g => g.toString().trim())
            meta.push(displayGenres.join(" "))
        }

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
                rating: item.rating || "",
                id: item.id || "",
                imdbId: item.imdbId || "",
                tmdbId: item.tmdbId || ""
            })
        }
    }

    function onError(message) {
        console.error("[HomeScreen] Error:", message)
        isLoading = false
    }
    
    function onHeroPlayClicked() {
        if (heroItems.length === 0 || currentHeroIndex < 0) return
        
        var heroItem = heroItems[currentHeroIndex]
        streamDialog.loadStreams(heroItem, "")
        streamDialog.open()
    }
    
    function onHeroAddToLibraryClicked() {
        if (heroItems.length === 0 || currentHeroIndex < 0) return

        var heroItem = heroItems[currentHeroIndex]
        var contentId = heroItem.imdbId || heroItem.id || ""
        var type = heroItem.type || "movie"

        if (!contentId) return

        // Prefer IMDB format
        if (heroItem.imdbId && heroItem.imdbId.startsWith("tt")) {
            contentId = heroItem.imdbId
        }

        // Normalize type for internal services
        if (type === "tv" || type === "series") type = "show"
        else type = "movie"

        if (traktAuthService.isAuthenticated) {
            traktWatchlistService.addToWatchlist(type, contentId)
        } else {
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

    function onHeroMoreInfoClicked() {
        if (heroItems.length === 0 || currentHeroIndex < 0) return

        var heroItem = heroItems[currentHeroIndex]
        var contentId = heroItem.imdbId || heroItem.id || ""
        var type = heroItem.type || "movie"
        var addonId = heroItem.addonId || ""

        if (!contentId) return

        if (heroItem.imdbId && heroItem.imdbId.startsWith("tt")) {
            contentId = heroItem.imdbId
        }

        if (type === "tv" || type === "series") type = "show"
        else type = "movie"

        navigationService.navigateToDetail(contentId, type, addonId || "")
        root.itemClicked(contentId, type, addonId)  // Keep for backward compatibility
    }

    Rectangle {
        anchors.fill: parent
        color: "#09090b"
        focus: true
        Keys.onRightPressed: cycleHero(1)
        Keys.onLeftPressed: cycleHero(-1)

        ScrollView {
            anchors.fill: parent
            contentWidth: width
            contentHeight: contentColumn.height
            clip: true

            Column {
                id: contentColumn
                width: parent.width
                spacing: 40
                bottomPadding: 80
                
                Loader {
                    id: heroLoader
                    width: parent.width
                    height: 650
                    source: "qrc:/qml/components/HeroSection.qml"
                    
                    onLoaded: {
                        if (item) {
                            item.playClicked.connect(() => root.onHeroPlayClicked())
                            item.moreInfoClicked.connect(() => root.onHeroMoreInfoClicked())
                        }
                    }
                    
                    // Left navigation arrow
                    Item {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: 60
                        visible: catalogSectionsModel.count > 0 && catalogSectionsModel.get(0).itemsModel.count > 1
                        z: 200

                        property bool isHovered: false

                        Item {
                            anchors.centerIn: parent
                            width: 48
                            height: 48
                            opacity: parent.isHovered ? 1.0 : 0.4
                            Behavior on opacity { NumberAnimation { duration: 200 } }

                            Image {
                                id: leftArrowIcon1
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
                            onClicked: root.cycleHero(-1)
                        }
                    }

                    // Right navigation arrow
                    Item {
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: 60
                        visible: catalogSectionsModel.count > 0 && catalogSectionsModel.get(0).itemsModel.count > 1
                        z: 200

                        property bool isHovered: false

                        Item {
                            anchors.centerIn: parent
                            width: 48
                            height: 48
                            opacity: parent.isHovered ? 1.0 : 0.4
                            Behavior on opacity { NumberAnimation { duration: 200 } }

                            Image {
                                id: rightArrowIcon1
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
                            onClicked: root.cycleHero(1)
                        }
                    }
                }
                
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
                        item.itemClicked.connect(function(contentId, type, addonId) {
                            navigationService.navigateToDetail(contentId, type, addonId || "")
        root.itemClicked(contentId, type, addonId)  // Keep for backward compatibility
                        })
                    }
                }

                Repeater {
                    id: catalogRepeater
                    model: catalogSectionsModel

                    Column {
                        id: sectionColumn
                        width: parent.width
                        spacing: 15

                        readonly property string sectionTitle: model.sectionTitle
                        readonly property var sectionItemsModel: model.itemsModel
                        
                        Text {
                            width: parent.width
                            height: 30
                            text: sectionColumn.sectionTitle
                            color: "white"
                            font.pixelSize: 18
                            font.bold: true
                            leftPadding: 50
                            verticalAlignment: Text.AlignVCenter
                        }

                        // Constrain width to apply margins
                        Item {
                            width: parent.width - 100
                            height: 420 
                            anchors.horizontalCenter: parent.horizontalCenter
                            
                            ListView {
                                id: sectionListView
                                anchors.fill: parent
                                orientation: ListView.Horizontal
                                spacing: 24 
                                clip: true 
                                model: sectionColumn.sectionItemsModel
                                
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
                                            var contentId = ""
                                            
                                            // Use ID as-is from catalog (preserves formats like "tmdb:123" for Stremio compatibility)
                                            contentId = model.id || model.tmdbId || model.imdbId || ""
                                            
                                            if (!contentId) {
                                                console.error("[HomeScreen] No contentId found for item:", model.title)
                                                return
                                            }
                                            
                                            root.itemClicked(contentId, model.type || "", addonId)
                                        }
                                    }

                                    Column {
                                        anchors.fill: parent
                                        spacing: 12

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

                                            Rectangle {
                                                anchors.fill: parent; color: "transparent"; radius: 8; z: 10
                                                border.width: 3; border.color: isHovered ? "#ffffff" : "transparent"
                                                Behavior on border.color { ColorAnimation { duration: 200 } }
                                            }
                                        }

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
                            
                            // Left scroll arrow
                            Item {
                                anchors.left: parent.left
                                anchors.verticalCenter: parent.verticalCenter
                                width: 48
                                height: 48
                                visible: sectionListView.contentX > 0
                                
                                property bool isHovered: false
                                
                                Item {
                                    anchors.centerIn: parent
                                    width: 48
                                    height: 48
                                    opacity: parent.isHovered ? 1.0 : 0.4
                                    Behavior on opacity { NumberAnimation { duration: 200 } }

                                    Image {
                                        id: leftArrowIcon2
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
                                        var targetX = Math.max(0, sectionListView.contentX - sectionListView.width * 0.8)
                                        sectionScrollAnimation.to = targetX
                                        sectionScrollAnimation.start()
                                    }
                                }
                            }
                            
                            // Right scroll arrow
                            Item {
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                width: 48
                                height: 48
                                visible: sectionListView.contentX < sectionListView.contentWidth - sectionListView.width
                                
                                property bool isHovered: false
                                
                                Item {
                                    anchors.centerIn: parent
                                    width: 48
                                    height: 48
                                    opacity: parent.isHovered ? 1.0 : 0.4
                                    Behavior on opacity { NumberAnimation { duration: 200 } }

                                    Image {
                                        id: rightArrowIcon2
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
                                            sectionListView.contentWidth - sectionListView.width,
                                            sectionListView.contentX + sectionListView.width * 0.8
                                        )
                                        sectionScrollAnimation.to = targetX
                                        sectionScrollAnimation.start()
                                    }
                                }
                            }
                        }
                    }
                }

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
    
    StreamSelectionDialog {
        id: streamDialog
        parent: root.parent
        
        onStreamSelected: function(stream) {
            console.log("[HomeScreen] Stream selected:", JSON.stringify(stream))
            if (stream.url) {
                navigationService.navigateToPlayer(stream.url, {})
                root.playRequested(stream.url)  // Keep for backward compatibility
            }
        }
    }
    
    signal playRequested(string streamUrl)
}