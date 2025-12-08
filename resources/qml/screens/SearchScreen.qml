import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import Yantrium.Services 1.0

Item {
    id: root

    property LibraryService libraryService: LibraryService
    signal itemClicked(string contentId, string type, string addonId)

    property bool isLoading: false
    property string currentQuery: ""
    property string searchQuery: ""

    ListModel { 
        id: searchSectionsModel 
    }

    Component.onCompleted: {
        console.warn("[SearchScreen] Component Loaded")
    }

    Rectangle {
        anchors.fill: parent
        color: "#09090b"

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // Header Section
            Column {
                Layout.fillWidth: true
                Layout.topMargin: 40
                Layout.bottomMargin: 30
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
            }

            // Results Area
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                Column {
                    width: parent.width
                    spacing: 30
                    topPadding: 20
                    bottomPadding: 50

                    Repeater {
                        id: searchSectionsRepeater
                        model: searchSectionsModel

                        Column {
                            id: sectionColumn
                            width: parent.width
                            spacing: 15

                            readonly property string sectionTitle: model.sectionTitle
                            readonly property var sectionItemsModel: model.itemsModel
                            readonly property string sectionAddonId: model.addonId || ""

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

                                    delegate: Item {
                                        width: 240
                                        height: 400

                                        property bool isHovered: false

                                        MouseArea {
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            onEntered: isHovered = true
                                            onExited: isHovered = false
                                            onClicked: {
                                                var contentId = model.id || model.imdbId || model.tmdbId || ""
                                                var type = model.type || ""
                                                var addonId = model.addonId || sectionColumn.sectionAddonId || ""
                                                root.itemClicked(contentId, type, addonId)
                                            }
                                            cursorShape: Qt.PointingHandCursor
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
                                                    anchors.fill: parent
                                                    source: model.posterUrl || model.poster || ""
                                                    fillMode: Image.PreserveAspectCrop
                                                    asynchronous: true
                                                    smooth: true
                                                    scale: isHovered ? 1.10 : 1.0
                                                    Behavior on scale { NumberAnimation { duration: 300; easing.type: Easing.OutQuad } }
                                                }

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

                                            Text {
                                                width: parent.width
                                                text: model.title || model.name || ""
                                                color: "white"
                                                font.pixelSize: 14
                                                font.bold: true
                                                elide: Text.ElideRight
                                                horizontalAlignment: Text.AlignHCenter
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: parent.width - 100
                        text: {
                            if (isLoading) return "Searching..."
                            if (currentQuery.length > 0 && searchSectionsModel.count === 0) return "No results found for \"" + currentQuery + "\""
                            return "Enter a search query"
                        }
                        font.pixelSize: 24
                        color: "#aaaaaa"
                        horizontalAlignment: Text.AlignHCenter
                        visible: searchSectionsModel.count === 0
                    }
                }
            }
        }
    }

    Connections {
        target: LibraryService
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
        
        for (var i = 0; i < searchSectionsModel.count; i++) {
            if (searchSectionsModel.get(i).sectionTitle === section.name) return
        }

        var cardModel = Qt.createQmlObject('import QtQuick; ListModel {}', searchSectionsModel)

        var itemsArray = Array.isArray(items) ? items : []
        if (!Array.isArray(items)) {
            try { for (var k = 0; k < items.length; k++) itemsArray.push(items[k]) } catch(e){}
        }

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

        searchSectionsModel.append({
            sectionTitle: section.name || "Unknown Section",
            addonId: section.addonId || "",
            itemsModel: cardModel
        })
    }

    function performSearch() {
        var query = searchQuery || ""
        if (query.length === 0) return

        if (root.currentQuery !== query) {
            searchSectionsModel.clear()
        }

        root.isLoading = true
        root.currentQuery = query

        if (typeof libraryService.searchTmdb === 'function') {
            libraryService.searchTmdb(query)
        } else {
            root.isLoading = false
        }
        searchQuery = ""
    }

    onSearchQueryChanged: {
        if (searchQuery && searchQuery.length > 0) {
            performSearch()
        }
    }
}
