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
            anchors.margins: 20

            // Addons Tab
            ScrollView {
                id: addonTab
                anchors.fill: parent
                visible: tabBar.currentIndex === 0
                clip: true
                
                Column {
                    id: addonColumn
                    width: parent.width
                    spacing: 15

                        Text {
                            text: "Addon Management"
                            font.pixelSize: 28
                            font.bold: true
                            color: "#FFFFFF"
                        }

                        property AddonRepository addonRepo: AddonRepository
                        
                        Connections {
                            target: addonRepo
                            function onAddonInstalled() {
                                statusText.text = "✓ Addon installed successfully!"
                                statusText.color = "#4CAF50"
                                addonCount.text = "Total Addons: " + addonRepo.listAddonsCount()
                                enabledCount.text = "Enabled: " + addonRepo.getEnabledAddonsCount()
                            }
                            
                            function onAddonUpdated() {
                                statusText.text = "✓ Addon updated successfully!"
                                statusText.color = "#2196F3"
                                addonCount.text = "Total Addons: " + addonRepo.listAddonsCount()
                                enabledCount.text = "Enabled: " + addonRepo.getEnabledAddonsCount()
                            }
                            
                            function onAddonRemoved(addonId) {
                                statusText.text = "✓ Removed addon: " + addonId
                                statusText.color = "#FF9800"
                                addonCount.text = "Total Addons: " + addonRepo.listAddonsCount()
                                enabledCount.text = "Enabled: " + addonRepo.getEnabledAddonsCount()
                            }
                            
                            function onError(errorMessage) {
                                statusText.text = "✗ Error: " + errorMessage
                                statusText.color = "#F44336"
                            }
                        }

                        // Status display
                        Rectangle {
                            width: parent.width
                    height: 60
                    color: "#1a1a1a"
                    radius: 8
                    border.color: "#333333"
                    border.width: 1

                    Column {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: 10
                        spacing: 5

                        Text {
                            id: statusText
                            text: "Ready"
                            color: "#FFFFFF"
                            font.pixelSize: 14
                        }

                        Row {
                            spacing: 20
                            Text {
                                id: addonCount
                                text: "Total Addons: " + addonRepo.listAddonsCount()
                                color: "#AAAAAA"
                                font.pixelSize: 12
                            }
                            Text {
                                id: enabledCount
                                text: "Enabled: " + addonRepo.getEnabledAddonsCount()
                                color: "#AAAAAA"
                                font.pixelSize: 12
                            }
                        }
                    }
                }

                        // Install section
                        Rectangle {
                            width: parent.width
                    height: 120
                    color: "#1a1a1a"
                    radius: 8
                    border.color: "#333333"
                    border.width: 1

                    Column {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        Text {
                            text: "Install Addon"
                            color: "#FFFFFF"
                            font.pixelSize: 16
                            font.bold: true
                        }

                        Row {
                            width: parent.width
                            spacing: 10

                            TextField {
                                id: manifestUrlInput
                                width: parent.width - installButton.width - parent.spacing
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

                            Button {
                                id: installButton
                                text: "Install"
                                onClicked: {
                                    if (manifestUrlInput.text.trim() !== "") {
                                        statusText.text = "Installing..."
                                        statusText.color = "#FFA500"
                                        addonRepo.installAddon(manifestUrlInput.text.trim())
                                    }
                                }
                            }
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
                    }
                }

                        // Addon Operations
                        Rectangle {
                            width: parent.width
                    height: 100
                    color: "#1a1a1a"
                    radius: 8
                    border.color: "#333333"
                    border.width: 1

                    Row {
                        anchors.centerIn: parent
                        spacing: 10

                        Button {
                            text: "Refresh Count"
                            onClicked: {
                                addonCount.text = "Total Addons: " + addonRepo.listAddonsCount()
                                enabledCount.text = "Enabled: " + addonRepo.getEnabledAddonsCount()
                            }
                        }

                        Button {
                            text: "View Addon Details"
                            onClicked: {
                                var json = addonRepo.getAddonJson("com.cinemeta.movie")
                                if (json) {
                                    statusText.text = "Found addon: " + json
                                    statusText.color = "#4CAF50"
                                } else {
                                    statusText.text = "Addon not found (try installing Cinemeta first)"
                                    statusText.color = "#FF9800"
                                }
                            }
                        }
                    }
                }
                }
            }

            // Trakt Tab
            ScrollView {
                id: traktTab
                anchors.fill: parent
                visible: tabBar.currentIndex === 1
                clip: true
                
                Column {
                    id: traktColumn
                    width: parent.width
                    spacing: 15

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
                        height: 60
                        color: "#1a1a1a"
                        radius: 8
                        border.color: "#333333"
                        border.width: 1

                        Column {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.margins: 10
                            spacing: 5

                            Text {
                                id: traktStatusText
                                text: "Ready"
                                color: "#FFFFFF"
                                font.pixelSize: 14
                            }
                        }
                    }
                }
            }

            // Trakt Tab
            ScrollView {
                id: traktTab
                anchors.fill: parent
                visible: tabBar.currentIndex === 1
                clip: true
                
                Column {
                    id: traktColumn
                    width: parent.width
                    spacing: 15

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

    Component.onCompleted: {
        addonCount.text = "Total Addons: " + addonRepo.listAddonsCount()
        enabledCount.text = "Enabled: " + addonRepo.getEnabledAddonsCount()
    }
}
