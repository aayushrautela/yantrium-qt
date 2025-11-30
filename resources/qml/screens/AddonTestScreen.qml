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
                text: "TMDB"
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

            // Addon Test Tab
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
                            text: "Addon System Test"
                            font.pixelSize: 28
                            font.bold: true
                            color: "#FFFFFF"
                        }

                        AddonRepository {
                            id: addonRepo
                            
                            onAddonInstalled: {
                                statusText.text = "✓ Addon installed successfully!"
                                statusText.color = "#4CAF50"
                                addonCount.text = "Total Addons: " + addonRepo.listAddonsCount()
                                enabledCount.text = "Enabled: " + addonRepo.getEnabledAddonsCount()
                            }
                            
                            onAddonUpdated: {
                                statusText.text = "✓ Addon updated successfully!"
                                statusText.color = "#2196F3"
                                addonCount.text = "Total Addons: " + addonRepo.listAddonsCount()
                                enabledCount.text = "Enabled: " + addonRepo.getEnabledAddonsCount()
                            }
                            
                            onAddonRemoved: function(addonId) {
                                statusText.text = "✓ Removed addon: " + addonId
                                statusText.color = "#FF9800"
                                addonCount.text = "Total Addons: " + addonRepo.listAddonsCount()
                                enabledCount.text = "Enabled: " + addonRepo.getEnabledAddonsCount()
                            }
                            
                            onError: function(errorMessage) {
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
                            text: "Ready to test"
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

                        // Test operations
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
                            text: "Test Get Addon"
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

            // TMDB Test Tab
            ScrollView {
                id: tmdbTab
                anchors.fill: parent
                visible: tabBar.currentIndex === 1
                clip: true
                
                Column {
                    id: tmdbColumn
                    width: parent.width
                    spacing: 15

                        Text {
                            text: "TMDB Integration Test"
                            font.pixelSize: 28
                            font.bold: true
                            color: "#FFFFFF"
                        }

                        TmdbSearchService {
                            id: tmdbSearch
                            
                            onMoviesFound: function(results) {
                        tmdbStatusText.text = "✓ Found " + results.length + " movies"
                        tmdbStatusText.color = "#4CAF50"
                        tmdbResultsModel.clear()
                        for (var i = 0; i < results.length; i++) {
                            var result = results[i]
                            tmdbResultsModel.append({
                                "id": result.id || 0,
                                "title": result.title || result.name || "",
                                "overview": result.overview || "",
                                "posterPath": result.posterPath || "",
                                "voteAverage": result.voteAverage || 0.0,
                                "popularity": result.popularity || 0.0
                            })
                        }
                            }
                            
                            onTvFound: function(results) {
                        tmdbStatusText.text = "✓ Found " + results.length + " TV shows"
                        tmdbStatusText.color = "#4CAF50"
                        tmdbResultsModel.clear()
                        for (var i = 0; i < results.length; i++) {
                            var result = results[i]
                            tmdbResultsModel.append({
                                "id": result.id || 0,
                                "title": result.name || result.title || "",
                                "overview": result.overview || "",
                                "posterPath": result.posterPath || "",
                                "voteAverage": result.voteAverage || 0.0,
                                "popularity": result.popularity || 0.0
                            })
                        }
                            }
                            
                            onError: function(errorMessage) {
                                tmdbStatusText.text = "✗ Error: " + errorMessage
                                tmdbStatusText.color = "#F44336"
                            }
                        }

                        TmdbService {
                            id: tmdbService
                            
                            onMovieMetadataFetched: function(tmdbId, data) {
                                tmdbStatusText.text = "✓ Movie metadata fetched for ID: " + tmdbId
                                tmdbStatusText.color = "#4CAF50"
                                tmdbMetadataText.text = JSON.stringify(data, null, 2)
                            }
                            
                            onTvMetadataFetched: function(tmdbId, data) {
                                tmdbStatusText.text = "✓ TV metadata fetched for ID: " + tmdbId
                                tmdbStatusText.color = "#4CAF50"
                                tmdbMetadataText.text = JSON.stringify(data, null, 2)
                            }
                            
                            onTmdbIdFound: function(imdbId, tmdbId) {
                                tmdbStatusText.text = "✓ Found TMDB ID: " + tmdbId + " for IMDB: " + imdbId
                                tmdbStatusText.color = "#4CAF50"
                            }
                            
                            onError: function(errorMessage) {
                                tmdbStatusText.text = "✗ Error: " + errorMessage
                                tmdbStatusText.color = "#F44336"
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
                            id: tmdbStatusText
                            text: "Ready to test TMDB"
                            color: "#FFFFFF"
                            font.pixelSize: 14
                        }
                    }
                }

                        // Search section
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
                            text: "TMDB Search"
                            color: "#FFFFFF"
                            font.pixelSize: 16
                            font.bold: true
                        }

                        Row {
                            width: parent.width
                            spacing: 10

                            TextField {
                                id: tmdbSearchInput
                                width: parent.width - searchMoviesButton.width - searchTvButton.width - parent.spacing * 2
                                placeholderText: "Enter search query (e.g., Matrix, Breaking Bad)"
                                color: "#FFFFFF"
                                background: Rectangle {
                                    color: "#2a2a2a"
                                    border.color: "#444444"
                                    border.width: 1
                                    radius: 4
                                }
                                text: "Matrix"
                            }

                            Button {
                                id: searchMoviesButton
                                text: "Search Movies"
                                onClicked: {
                                    if (tmdbSearchInput.text.trim() !== "") {
                                        tmdbStatusText.text = "Searching movies..."
                                        tmdbStatusText.color = "#FFA500"
                                        tmdbResultsModel.clear()
                                        tmdbSearch.searchMovies(tmdbSearchInput.text.trim())
                                    }
                                }
                            }

                            Button {
                                id: searchTvButton
                                text: "Search TV"
                                onClicked: {
                                    if (tmdbSearchInput.text.trim() !== "") {
                                        tmdbStatusText.text = "Searching TV shows..."
                                        tmdbStatusText.color = "#FFA500"
                                        tmdbResultsModel.clear()
                                        tmdbSearch.searchTv(tmdbSearchInput.text.trim())
                                    }
                                }
                            }
                        }

                        // Quick search buttons
                        Row {
                            spacing: 10
                            Button {
                                text: "Matrix"
                                onClicked: tmdbSearchInput.text = "Matrix"
                            }
                            Button {
                                text: "Breaking Bad"
                                onClicked: tmdbSearchInput.text = "Breaking Bad"
                            }
                            Button {
                                text: "Inception"
                                onClicked: tmdbSearchInput.text = "Inception"
                            }
                        }
                    }
                }

                        // Metadata fetch section
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

                        TextField {
                            id: tmdbIdInput
                            placeholderText: "Enter TMDB ID (e.g., 603 for The Matrix)"
                            color: "#FFFFFF"
                            background: Rectangle {
                                color: "#2a2a2a"
                                border.color: "#444444"
                                border.width: 1
                                radius: 4
                            }
                            width: 200
                            text: "603"
                        }

                        Button {
                            text: "Get Movie Metadata"
                            onClicked: {
                                var id = parseInt(tmdbIdInput.text)
                                if (id > 0) {
                                    tmdbStatusText.text = "Fetching movie metadata..."
                                    tmdbStatusText.color = "#FFA500"
                                    tmdbService.getMovieMetadata(id)
                                }
                            }
                        }

                        Button {
                            text: "Get TV Metadata"
                            onClicked: {
                                var id = parseInt(tmdbIdInput.text)
                                if (id > 0) {
                                    tmdbStatusText.text = "Fetching TV metadata..."
                                    tmdbStatusText.color = "#FFA500"
                                    tmdbService.getTvMetadata(id)
                                }
                            }
                        }

                        Button {
                            text: "IMDB to TMDB"
                            onClicked: {
                                var imdbId = tmdbIdInput.text
                                if (imdbId.startsWith("tt")) {
                                    tmdbStatusText.text = "Looking up TMDB ID..."
                                    tmdbStatusText.color = "#FFA500"
                                    tmdbService.getTmdbIdFromImdb(imdbId)
                                } else {
                                    tmdbStatusText.text = "Enter IMDB ID (starts with 'tt')"
                                    tmdbStatusText.color = "#FF9800"
                                }
                            }
                        }
                    }
                }

                        // Results section
                        Rectangle {
                            width: parent.width
                            height: 400
                            color: "#1a1a1a"
                            radius: 8
                            border.color: "#333333"
                            border.width: 1

                    Column {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        Text {
                            text: "Search Results / Metadata"
                            color: "#FFFFFF"
                            font.pixelSize: 16
                            font.bold: true
                        }

                        ScrollView {
                            width: parent.width
                            height: parent.height - 40
                            clip: true

                            Column {
                                width: parent.width
                                spacing: 10

                                // Search results list
                                Repeater {
                                    model: ListModel {
                                        id: tmdbResultsModel
                                    }
                                    delegate: Rectangle {
                                        width: parent.width
                                        height: 80
                                        color: "#2a2a2a"
                                        radius: 4
                                        border.color: "#444444"
                                        border.width: 1

                                        Row {
                                            anchors.fill: parent
                                            anchors.margins: 10
                                            spacing: 10

                                            Text {
                                                width: 60
                                                text: "ID: " + model.id
                                                color: "#AAAAAA"
                                                font.pixelSize: 11
                                            }

                                            Column {
                                                width: parent.width - 200
                                                spacing: 5

                                                Text {
                                                    text: model.title || "Unknown"
                                                    color: "#FFFFFF"
                                                    font.pixelSize: 14
                                                    font.bold: true
                                                    elide: Text.ElideRight
                                                    width: parent.width
                                                }
                                                Text {
                                                    text: model.overview || ""
                                                    color: "#AAAAAA"
                                                    font.pixelSize: 11
                                                    elide: Text.ElideRight
                                                    width: parent.width
                                                    maximumLineCount: 2
                                                }
                                            }

                                            Column {
                                                width: 100
                                                spacing: 5

                                                Text {
                                                    text: "Rating: " + model.voteAverage.toFixed(1)
                                                    color: "#AAAAAA"
                                                    font.pixelSize: 11
                                                }
                                                Text {
                                                    text: "Popularity: " + model.popularity.toFixed(1)
                                                    color: "#AAAAAA"
                                                    font.pixelSize: 11
                                                }
                                            }
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: {
                                                tmdbIdInput.text = model.id
                                                tmdbService.getMovieMetadata(model.id)
                                            }
                                        }
                                    }
                                }

                                // Metadata display
                                Text {
                                    id: tmdbMetadataText
                                    width: parent.width
                                    visible: text !== ""
                                    color: "#AAAAAA"
                                    font.pixelSize: 10
                                    font.family: "monospace"
                                    wrapMode: Text.Wrap
                                }
                            }
                        }
                    }
                }
                }
            }

            // Trakt Test Tab
            ScrollView {
                id: traktTab
                anchors.fill: parent
                visible: tabBar.currentIndex === 2
                clip: true
                
                Column {
                    id: traktColumn
                    width: parent.width
                    spacing: 15

                    Text {
                        text: "Trakt Integration Test"
                        font.pixelSize: 28
                        font.bold: true
                        color: "#FFFFFF"
                    }

                    TraktAuthService {
                        id: traktAuth
                        
                        onDeviceCodeGenerated: function(userCode, verificationUrl, expiresIn) {
                            traktStatusText.text = "✓ Device code generated!\nUser Code: " + userCode + "\nVisit: " + verificationUrl
                            traktStatusText.color = "#4CAF50"
                            traktUserCodeText.text = "User Code: " + userCode
                            traktVerificationUrlText.text = "Visit: " + verificationUrl
                        }
                        
                        onAuthenticationStatusChanged: function(authenticated) {
                            if (authenticated) {
                                traktStatusText.text = "✓ Authenticated with Trakt"
                                traktStatusText.color = "#4CAF50"
                            } else {
                                traktStatusText.text = "Not authenticated"
                                traktStatusText.color = "#FF9800"
                            }
                        }
                        
                        onUserInfoFetched: function(username, slug) {
                            traktStatusText.text = "✓ User: " + username + " (" + slug + ")"
                            traktStatusText.color = "#4CAF50"
                        }
                        
                        onError: function(errorMessage) {
                            traktStatusText.text = "✗ Error: " + errorMessage
                            traktStatusText.color = "#F44336"
                        }
                    }

                    TraktScrobbleService {
                        id: traktScrobble
                        
                        onScrobbleStarted: function(success) {
                            traktStatusText.text = success ? "✓ Scrobble started" : "✗ Failed to start scrobble"
                            traktStatusText.color = success ? "#4CAF50" : "#F44336"
                        }
                        
                        onScrobblePaused: function(success) {
                            traktStatusText.text = success ? "✓ Scrobble paused" : "✗ Failed to pause scrobble"
                            traktStatusText.color = success ? "#4CAF50" : "#F44336"
                        }
                        
                        onScrobbleStopped: function(success) {
                            traktStatusText.text = success ? "✓ Scrobble stopped" : "✗ Failed to stop scrobble"
                            traktStatusText.color = success ? "#4CAF50" : "#F44336"
                        }
                        
                        onHistoryFetched: function(history) {
                            traktStatusText.text = "✓ Fetched " + history.length + " history items"
                            traktStatusText.color = "#4CAF50"
                        }
                        
                        onError: function(errorMessage) {
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
                                text: "Ready to test Trakt"
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
                                    enabled: traktAuth.isConfigured
                                    onClicked: {
                                        traktStatusText.text = "Generating device code..."
                                        traktStatusText.color = "#FFA500"
                                        traktAuth.generateDeviceCode()
                                    }
                                }

                                Button {
                                    text: "Check Auth Status"
                                    onClicked: {
                                        traktAuth.checkAuthentication()
                                    }
                                }

                                Button {
                                    text: "Get User Info"
                                    onClicked: {
                                        traktAuth.getCurrentUser()
                                    }
                                }

                                Button {
                                    text: "Logout"
                                    onClicked: {
                                        traktAuth.logout()
                                    }
                                }
                            }

                            Text {
                                text: "Configured: " + (traktAuth.isConfigured ? "Yes" : "No") + " | Authenticated: " + (traktAuth.isAuthenticated ? "Yes" : "No")
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
                                    text: "Get Movies"
                                    onClicked: traktWatchlist.getWatchlistMoviesWithImages()
                                }

                                Button {
                                    text: "Get Shows"
                                    onClicked: traktWatchlist.getWatchlistShowsWithImages()
                                }

                                Button {
                                    text: "Add"
                                    onClicked: {
                                        traktWatchlist.addToWatchlist(watchlistTypeInput.text, watchlistImdbInput.text)
                                    }
                                }

                                Button {
                                    text: "Remove"
                                    onClicked: {
                                        traktWatchlist.removeFromWatchlist(watchlistTypeInput.text, watchlistImdbInput.text)
                                    }
                                }
                            }
                        }
                    }

                    // Collection section
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
                                text: "Collection"
                                color: "#FFFFFF"
                                font.pixelSize: 16
                                font.bold: true
                            }

                            Row {
                                width: parent.width
                                spacing: 10

                                Button {
                                    text: "Get Movies"
                                    onClicked: traktWatchlist.getCollectionMoviesWithImages()
                                }

                                Button {
                                    text: "Get Shows"
                                    onClicked: traktWatchlist.getCollectionShowsWithImages()
                                }

                                Button {
                                    text: "Add"
                                    onClicked: {
                                        traktWatchlist.addToCollection(watchlistTypeInput.text, watchlistImdbInput.text)
                                    }
                                }

                                Button {
                                    text: "Remove"
                                    onClicked: {
                                        traktWatchlist.removeFromCollection(watchlistTypeInput.text, watchlistImdbInput.text)
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
