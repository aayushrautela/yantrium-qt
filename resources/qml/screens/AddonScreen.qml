import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Yantrium.Components 1.0
import Yantrium.Services 1.0

Item {
    id: root
    anchors.fill: parent

    Rectangle {
        anchors.fill: parent
        color: "#09090b"

        TabBar {
            id: tabBar
            width: parent.width
            height: 40
            background: Rectangle {
                color: "#1a1a1a"
            }

            TabButton {
                text: "Addons"
                width: 150
            }
            TabButton {
                text: "Trakt"
                width: 150
            }
        }

        Item {
            id: contentArea
            anchors.top: tabBar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            // Addons Tab
            ScrollView {
                id: addonTab
                anchors.fill: parent
                visible: tabBar.currentIndex === 0
                clip: true
                
                Column {
                    id: addonColumn
                    width: parent.width
                    spacing: 20
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 20

                    // Header with title and add button
                    Row {
                        id: headerRow
                        width: parent.width
                        spacing: 20
                        
                        // Settings icon and title
                        Row {
                            id: settingsTitleRow
                            spacing: 12
                            anchors.verticalCenter: parent.verticalCenter
                            
                            Image {
                                source: "qrc:/assets/icons/settings.svg"
                                width: 24
                                height: 24
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            
                            Text {
                                text: "Addon Management"
                                font.pixelSize: 28
                                font.bold: true
                                color: "#FFFFFF"
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                        
                        Item {
                            width: parent.width - settingsTitleRow.width - addButton.width - parent.spacing
                            height: 1
                        }
                        
                        // Add button
                        Rectangle {
                            id: addButton
                            width: 40
                            height: 40
                            radius: 20
                            color: mouseArea.containsMouse ? "#2a2a2a" : "#1a1a1a"
                            border.color: "#333333"
                            border.width: 1
                            
                            Text {
                                text: "+"
                                font.pixelSize: 28
                                font.bold: true
                                color: "#FFFFFF"
                                anchors.centerIn: parent
                            }
                            
                            MouseArea {
                                id: mouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    installDialog.open()
                                }
                            }
                        }
                    }
                    
                    property AddonRepository addonRepo: AddonRepository
                    
                    // Addon list model
                    ListModel {
                        id: addonListModel
                    }
                    
                    function refreshAddonList() {
                        addonListModel.clear()
                        var addons = addonRepo.getAllAddons()
                        for (var i = 0; i < addons.length; i++) {
                            addonListModel.append(addons[i])
                        }
                    }
                    
                    Connections {
                        target: addonRepo
                        function onAddonInstalled() {
                            refreshAddonList()
                        }
                        
                        function onAddonUpdated() {
                            refreshAddonList()
                        }
                        
                        function onAddonRemoved(addonId) {
                            refreshAddonList()
                        }
                    }
                    
                    // Addon cards in Flow layout
                    Flow {
                        width: parent.width
                        spacing: 20
                        
                        Repeater {
                            model: addonListModel
                            
                            Rectangle {
                                id: addonCard
                                width: Math.max(280, Math.min(350, (parent.width - 40) / 3))
                                height: cardContent.height + 32
                                color: "#1a1a1a"
                                radius: 8
                                border.color: "#2d2d2d"
                                border.width: 1
                                
                                Column {
                                    id: cardContent
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.top: parent.top
                                    anchors.margins: 16
                                    spacing: 12
                                    
                                    // Addon name (large, bold)
                                    Text {
                                        width: parent.width
                                        text: model.name || model.id || "Unknown Addon"
                                        font.pixelSize: 20
                                        font.bold: true
                                        color: "#FFFFFF"
                                        elide: Text.ElideRight
                                    }
                                    
                                    // Version
                                    Text {
                                        text: "Version: " + (model.version || "N/A")
                                        font.pixelSize: 14
                                        color: "#AAAAAA"
                                    }
                                    
                                    // Description
                                    Text {
                                        width: parent.width
                                        text: model.description || "No description available"
                                        font.pixelSize: 13
                                        color: "#AAAAAA"
                                        wrapMode: Text.WordWrap
                                        maximumLineCount: 3
                                        elide: Text.ElideRight
                                    }
                                    
                                    // Controls row (toggle and delete)
                                    Row {
                                        width: parent.width
                                        spacing: 12
                                        
                                        Item {
                                            width: parent.width - toggleSwitch.width - deleteButton.width - parent.spacing
                                            height: 1
                                        }
                                        
                                        // Toggle switch
                                        Rectangle {
                                            id: toggleSwitch
                                            width: 50
                                            height: 28
                                            radius: 14
                                            color: (model.enabled || false) ? "#E53935" : "#666666"
                                            
                                            Behavior on color {
                                                ColorAnimation { duration: 200 }
                                            }
                                            
                                            Rectangle {
                                                id: toggleHandle
                                                width: 24
                                                height: 24
                                                radius: 12
                                                color: "#FFFFFF"
                                                anchors.verticalCenter: parent.verticalCenter
                                                anchors.left: parent.left
                                                anchors.leftMargin: (model.enabled || false) ? 24 : 2
                                                
                                                Behavior on anchors.leftMargin {
                                                    NumberAnimation { duration: 200 }
                                                }
                                            }
                                            
                                            MouseArea {
                                                anchors.fill: parent
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: {
                                                    if (model.enabled) {
                                                        addonColumn.addonRepo.disableAddon(model.id)
                                                    } else {
                                                        addonColumn.addonRepo.enableAddon(model.id)
                                                    }
                                                    addonColumn.refreshAddonList()
                                                }
                                            }
                                        }
                                        
                                        // Delete button
                                        Rectangle {
                                            id: deleteButton
                                            width: 32
                                            height: 32
                                            radius: 16
                                            color: deleteMouseArea.containsMouse ? "#2a2a2a" : "transparent"
                                            
                                            Image {
                                                source: "qrc:/assets/icons/delete.svg"
                                                width: 20
                                                height: 20
                                                anchors.centerIn: parent
                                            }
                                            
                                            MouseArea {
                                                id: deleteMouseArea
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: {
                                                    addonColumn.addonRepo.removeAddon(model.id)
                                                    addonColumn.refreshAddonList()
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Trakt Tab (keeping existing implementation)
            ScrollView {
                id: traktTab
                anchors.fill: parent
                visible: tabBar.currentIndex === 1
                clip: true
                
                Column {
                    id: traktColumn
                    width: parent.width
                    spacing: 15
                    anchors.margins: 20

                    Text {
                        text: "Trakt Integration"
                        font.pixelSize: 28
                        font.bold: true
                        color: "#FFFFFF"
                    }

                    // TraktAuthService is a singleton, accessed directly
                    Connections {
                        target: TraktAuthService
                        
                        function onDeviceCodeGenerated(userCode, verificationUrl, expiresIn) {
                            traktStatusText.text = "✓ Device code generated!\nUser Code: " + userCode + "\nVisit: " + verificationUrl
                            traktStatusText.color = "#4CAF50"
                            traktUserCodeText.text = "User Code: " + userCode
                            traktVerificationUrlText.text = "Visit: " + verificationUrl
                        }
                        
                        function onAuthenticationStatusChanged(authenticated) {
                            if (authenticated) {
                                traktStatusText.text = "✓ Authenticated with Trakt"
                                traktStatusText.color = "#4CAF50"
                            } else {
                                traktStatusText.text = "Not authenticated"
                                traktStatusText.color = "#FF9800"
                            }
                        }
                        
                        function onUserInfoFetched(username, slug) {
                            traktStatusText.text = "✓ User: " + username + " (" + slug + ")"
                            traktStatusText.color = "#4CAF50"
                        }
                        
                        function onError(errorMessage) {
                            traktStatusText.text = "✗ Error: " + errorMessage
                            traktStatusText.color = "#F44336"
                        }
                    }

                    property TraktScrobbleService traktScrobble: TraktScrobbleService
                    
                    Connections {
                        target: traktScrobble
                        function onScrobbleStarted(success) {
                            traktStatusText.text = success ? "✓ Scrobble started" : "✗ Failed to start scrobble"
                            traktStatusText.color = success ? "#4CAF50" : "#F44336"
                        }
                        
                        function onScrobblePaused(success) {
                            traktStatusText.text = success ? "✓ Scrobble paused" : "✗ Failed to pause scrobble"
                            traktStatusText.color = success ? "#4CAF50" : "#F44336"
                        }
                        
                        function onScrobbleStopped(success) {
                            traktStatusText.text = success ? "✓ Scrobble stopped" : "✗ Failed to stop scrobble"
                            traktStatusText.color = success ? "#4CAF50" : "#F44336"
                        }
                        
                        function onHistoryFetched(history) {
                            traktStatusText.text = "✓ Fetched " + history.length + " history items"
                            traktStatusText.color = "#4CAF50"
                        }
                        
                        function onError(errorMessage) {
                            traktStatusText.text = "✗ Error: " + errorMessage
                            traktStatusText.color = "#F44336"
                        }
                    }

                    TraktWatchlistService {
                        id: traktWatchlist
                        
                        onWatchlistMoviesFetched: function(movies) {
                            traktStatusText.text = "✓ Fetched " + movies.length + " watchlist movies"
                            traktStatusText.color = "#4CAF50"
                        }
                        
                        onWatchlistShowsFetched: function(shows) {
                            traktStatusText.text = "✓ Fetched " + shows.length + " watchlist shows"
                            traktStatusText.color = "#4CAF50"
                        }
                        
                        onCollectionMoviesFetched: function(movies) {
                            traktStatusText.text = "✓ Fetched " + movies.length + " collection movies"
                            traktStatusText.color = "#4CAF50"
                        }
                        
                        onCollectionShowsFetched: function(shows) {
                            traktStatusText.text = "✓ Fetched " + shows.length + " collection shows"
                            traktStatusText.color = "#4CAF50"
                        }
                        
                        onError: function(errorMessage) {
                            traktStatusText.text = "✗ Error: " + errorMessage
                            traktStatusText.color = "#F44336"
                        }
                    }

                    // Status display
                    Rectangle {
                        width: parent.width
                        height: 80
                        color: "#1a1a1a"
                        radius: 8
                        border.color: "#333333"
                        border.width: 1

                        Column {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 5

                            Text {
                                id: traktStatusText
                                text: "Ready"
                                color: "#FFFFFF"
                                font.pixelSize: 14
                                wrapMode: Text.Wrap
                                width: parent.width
                            }

                            Text {
                                id: traktUserCodeText
                                visible: text !== ""
                                color: "#AAAAAA"
                                font.pixelSize: 12
                            }

                            Text {
                                id: traktVerificationUrlText
                                visible: text !== ""
                                color: "#AAAAAA"
                                font.pixelSize: 12
                            }
                        }
                    }

                    // Authentication section
                    Rectangle {
                        width: parent.width
                        height: 200
                        color: "#1a1a1a"
                        radius: 8
                        border.color: "#333333"
                        border.width: 1

                        Column {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 10

                            Text {
                                text: "Authentication"
                                color: "#FFFFFF"
                                font.pixelSize: 16
                                font.bold: true
                            }

                            Row {
                                width: parent.width
                                spacing: 10

                                Button {
                                    text: "Generate Device Code"
                                    enabled: TraktAuthService.isConfigured
                                    onClicked: {
                                        traktStatusText.text = "Generating device code..."
                                        traktStatusText.color = "#FFA500"
                                        TraktAuthService.generateDeviceCode()
                                    }
                                }

                                Button {
                                    text: "Check Auth Status"
                                    onClicked: {
                                        TraktAuthService.checkAuthentication()
                                    }
                                }

                                Button {
                                    text: "Get User Info"
                                    onClicked: {
                                        TraktAuthService.getCurrentUser()
                                    }
                                }

                                Button {
                                    text: "Logout"
                                    onClicked: {
                                        TraktAuthService.logout()
                                    }
                                }
                            }

                            Text {
                                text: "Configured: " + (TraktAuthService.isConfigured ? "Yes" : "No") + " | Authenticated: " + (TraktAuthService.isAuthenticated ? "Yes" : "No")
                                color: "#AAAAAA"
                                font.pixelSize: 12
                            }
                        }
                    }

                    // Scrobbling section
                    Rectangle {
                        width: parent.width
                        height: 280
                        color: "#1a1a1a"
                        radius: 8
                        border.color: "#333333"
                        border.width: 1

                        Column {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 10

                            Text {
                                text: "Scrobbling"
                                color: "#FFFFFF"
                                font.pixelSize: 16
                                font.bold: true
                            }

                            Row {
                                width: parent.width
                                spacing: 10

                                TextField {
                                    id: scrobbleTypeInput
                                    width: 100
                                    placeholderText: "Type"
                                    text: "movie"
                                    color: "#FFFFFF"
                                    background: Rectangle {
                                        color: "#2a2a2a"
                                        border.color: "#444444"
                                        border.width: 1
                                        radius: 4
                                    }
                                }

                                TextField {
                                    id: scrobbleImdbInput
                                    width: 150
                                    placeholderText: "IMDb ID"
                                    text: "tt0133093"
                                    color: "#FFFFFF"
                                    background: Rectangle {
                                        color: "#2a2a2a"
                                        border.color: "#444444"
                                        border.width: 1
                                        radius: 4
                                    }
                                }

                                TextField {
                                    id: scrobbleTitleInput
                                    width: 200
                                    placeholderText: "Title"
                                    text: "The Matrix"
                                    color: "#FFFFFF"
                                    background: Rectangle {
                                        color: "#2a2a2a"
                                        border.color: "#444444"
                                        border.width: 1
                                        radius: 4
                                    }
                                }

                                TextField {
                                    id: scrobbleYearInput
                                    width: 80
                                    placeholderText: "Year"
                                    text: "1999"
                                    color: "#FFFFFF"
                                    background: Rectangle {
                                        color: "#2a2a2a"
                                        border.color: "#444444"
                                        border.width: 1
                                        radius: 4
                                    }
                                }
                            }

                            Row {
                                width: parent.width
                                spacing: 10

                                Slider {
                                    id: scrobbleProgressSlider
                                    width: 200
                                    from: 0
                                    to: 100
                                    value: 50
                                }

                                Text {
                                    text: Math.round(scrobbleProgressSlider.value) + "%"
                                    color: "#FFFFFF"
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }

                            Row {
                                width: parent.width
                                spacing: 10

                                Button {
                                    text: "Start"
                                    onClicked: {
                                        var contentData = {
                                            "type": scrobbleTypeInput.text,
                                            "imdbId": scrobbleImdbInput.text,
                                            "title": scrobbleTitleInput.text,
                                            "year": parseInt(scrobbleYearInput.text) || 0
                                        }
                                        traktScrobble.scrobbleStart(contentData, scrobbleProgressSlider.value)
                                    }
                                }

                                Button {
                                    text: "Pause"
                                    onClicked: {
                                        var contentData = {
                                            "type": scrobbleTypeInput.text,
                                            "imdbId": scrobbleImdbInput.text,
                                            "title": scrobbleTitleInput.text,
                                            "year": parseInt(scrobbleYearInput.text) || 0
                                        }
                                        traktScrobble.scrobblePause(contentData, scrobbleProgressSlider.value)
                                    }
                                }

                                Button {
                                    text: "Stop"
                                    onClicked: {
                                        var contentData = {
                                            "type": scrobbleTypeInput.text,
                                            "imdbId": scrobbleImdbInput.text,
                                            "title": scrobbleTitleInput.text,
                                            "year": parseInt(scrobbleYearInput.text) || 0
                                        }
                                        traktScrobble.scrobbleStop(contentData, scrobbleProgressSlider.value)
                                    }
                                }

                                Button {
                                    text: "Get History"
                                    onClicked: {
                                        traktScrobble.getHistoryMovies()
                                    }
                                }
                            }
                        }
                    }

                    // Watchlist section
                    Rectangle {
                        width: parent.width
                        height: 150
                        color: "#1a1a1a"
                        radius: 8
                        border.color: "#333333"
                        border.width: 1

                        Column {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 10

                            Text {
                                text: "Watchlist"
                                color: "#FFFFFF"
                                font.pixelSize: 16
                                font.bold: true
                            }

                            Row {
                                width: parent.width
                                spacing: 10

                                TextField {
                                    id: watchlistTypeInput
                                    width: 100
                                    placeholderText: "Type"
                                    text: "movie"
                                    color: "#FFFFFF"
                                    background: Rectangle {
                                        color: "#2a2a2a"
                                        border.color: "#444444"
                                        border.width: 1
                                        radius: 4
                                    }
                                }

                                TextField {
                                    id: watchlistImdbInput
                                    width: 150
                                    placeholderText: "IMDb ID"
                                    text: "tt0133093"
                                    color: "#FFFFFF"
                                    background: Rectangle {
                                        color: "#2a2a2a"
                                        border.color: "#444444"
                                        border.width: 1
                                        radius: 4
                                    }
                                }

                                Button {
                                    text: "Get Watchlist Movies"
                                    onClicked: {
                                        traktWatchlist.getWatchlistMovies()
                                    }
                                }

                                Button {
                                    text: "Get Watchlist Shows"
                                    onClicked: {
                                        traktWatchlist.getWatchlistShows()
                                    }
                                }

                                Button {
                                    text: "Get Collection Movies"
                                    onClicked: {
                                        traktWatchlist.getCollectionMovies()
                                    }
                                }

                                Button {
                                    text: "Get Collection Shows"
                                    onClicked: {
                                        traktWatchlist.getCollectionShows()
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Install Addon Dialog
    Dialog {
        id: installDialog
        title: "Install Addon"
        width: 600
        height: 300
        anchors.centerIn: parent
        
        Column {
            anchors.fill: parent
            spacing: 15
            
            Text {
                text: "Enter manifest URL"
                font.pixelSize: 16
                color: "#FFFFFF"
            }
            
            TextField {
                id: manifestUrlInput
                width: parent.width
                placeholderText: "Enter manifest URL"
                color: "#FFFFFF"
                background: Rectangle {
                    color: "#2a2a2a"
                    border.color: "#444444"
                    border.width: 1
                    radius: 4
                }
                text: "https://v3-cinemeta.strem.io/manifest.json"
            }
            
            // Quick install buttons
            Row {
                spacing: 10
                
                Button {
                    text: "Cinemeta"
                    onClicked: manifestUrlInput.text = "https://v3-cinemeta.strem.io/manifest.json"
                }
                Button {
                    text: "OpenSubtitles"
                    onClicked: manifestUrlInput.text = "https://opensubtitles-v3.strem.io/manifest.json"
                }
            }
            
            Row {
                spacing: 10
                
                Button {
                    text: "Install"
                    onClicked: {
                        if (manifestUrlInput.text.trim() !== "") {
                            addonColumn.addonRepo.installAddon(manifestUrlInput.text.trim())
                            installDialog.close()
                        }
                    }
                }
                
                Button {
                    text: "Cancel"
                    onClicked: installDialog.close()
                }
            }
        }
    }

    Component.onCompleted: {
        addonColumn.refreshAddonList()
    }
}
