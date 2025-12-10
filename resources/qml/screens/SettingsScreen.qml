import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Yantrium.Components 1.0
import Yantrium.Services 1.0

Item {
    id: root
    anchors.fill: parent
    
    // Services
    property AddonRepository addonRepo: AddonRepository
    property CatalogPreferencesService catalogPrefsService: CatalogPreferencesService
    property LibraryService libraryService: LibraryService
    property Configuration configuration: Configuration
    
    Rectangle {
        anchors.fill: parent
        color: "#09090b"
        
        ScrollView {
            anchors.fill: parent
            anchors.margins: 20
            contentWidth: width
            contentHeight: settingsColumn.height
            
            Column {
                id: settingsColumn
                width: parent.width
                spacing: 30
                
                // Title
                Text {
                    width: parent.width
                    text: "Settings"
                    font.pixelSize: 32
                    font.bold: true
                    color: "#ffffff"
                }
                
                // ==========================================
                // ADDONS SECTION
                // ==========================================
                Rectangle {
                    width: parent.width
                    height: addonsSection.height + 40
                    color: "#1a1a1a"
                    radius: 8
                    
                    Column {
                        id: addonsSection
                        width: parent.width - 40
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.margins: 20
                        spacing: 20
                        
                        // Header
                        Item {
                            width: parent.width
                            height: Math.max(headerColumn.height, badge.height)
                            
                            Column {
                                id: headerColumn
                                width: parent.width - (badge.visible ? badge.width + 20 : 0)
                                Text { text: "Addons"; font.pixelSize: 28; font.bold: true; color: "#ffffff" }
                                Text { text: "Manage your installed extensions and sources."; font.pixelSize: 14; color: "#aaaaaa"; wrapMode: Text.WordWrap; width: parent.width }
                            }
                            Rectangle {
                                id: badge
                                anchors.right: parent.right; anchors.top: parent.top; anchors.topMargin: 4
                                width: badgeText.width + 16; height: 24; radius: 12; color: "#2d2d2d"
                                visible: addonListModel.count > 0
                                Text { id: badgeText; anchors.centerIn: parent; text: addonListModel.count + " Installed"; font.pixelSize: 12; font.bold: true; color: "#aaaaaa" }
                            }
                        }
                        
                        Connections {
                            target: addonRepo
                            function onAddonInstalled(addon) { addonStatusText.text = "\u2713 Addon installed: " + addon.name; addonStatusText.color = "#4CAF50"; refreshAddonList() }
                            function onAddonRemoved(addonId) { addonStatusText.text = "\u2713 Removed addon: " + addonId; addonStatusText.color = "#FF9800"; refreshAddonList() }
                            function onError(errorMessage) { addonStatusText.text = "\u2717 Error: " + errorMessage; addonStatusText.color = "#F44336" }
                        }
                        
                        // Add new addon
                        Column {
                            width: parent.width
                            spacing: 10
                            Text { text: "Install Addon"; font.pixelSize: 16; font.bold: true; color: "#ffffff" }
                            
                            Row {
                                width: parent.width
                                spacing: 10
                                
                                Rectangle {
                                    width: parent.width - installButton.width - 10
                                    height: 44
                                    color: "#2d2d2d"
                                    border.width: 1
                                    border.color: manifestUrlField.activeFocus ? "#ffffff" : "#3d3d3d"
                                    radius: 4
                                    
                                    Row {
                                        anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter
                                        anchors.leftMargin: 12; anchors.rightMargin: 12; spacing: 8
                                        TextField {
                                            id: manifestUrlField
                                            width: parent.parent.width - 40; anchors.verticalCenter: parent.verticalCenter
                                            placeholderText: "Enter addon manifest URL (e.g., https://...)"; color: "#ffffff"; placeholderTextColor: "#666666"
                                            background: Rectangle { color: "transparent" }
                                        }
                                    }
                                }
                                
                                // --- INSTALL BUTTON ---
                                Button {
                                    id: installButton
                                    width: 44
                                    height: 44
                                    
                                    // 1. REMOVE BUTTON BACKGROUND (Makes it transparent)
                                    background: Rectangle {
                                        color: parent.pressed ? "#22ffffff" : "transparent"
                                        radius: 4
                                    }

                                    contentItem: Item {
                                        Image {
                                            id: installIcon
                                            source: "qrc:/assets/icons/add.svg"
                                            
                                            // 2. QUALITY FIX: High source size + mipmap + smooth
                                            sourceSize.width: 128
                                            sourceSize.height: 128
                                            mipmap: true  // CRITICAL for smooth downscaling
                                            smooth: true
                                            antialiasing: true
                                            
                                            // Display Size
                                            width: 36
                                            height: 36
                                            
                                            anchors.centerIn: parent
                                            fillMode: Image.PreserveAspectFit
                                        }
                                    }
                                    
                                    onClicked: {
                                        if (manifestUrlField.text.trim() !== "") {
                                            addonStatusText.text = "Installing..."; addonStatusText.color = "#2196F3"
                                            addonRepo.installAddon(manifestUrlField.text.trim())
                                            manifestUrlField.text = ""
                                        }
                                    }
                                }
                            }
                            Text { id: addonStatusText; width: parent.width; font.pixelSize: 14; color: "#aaaaaa"; wrapMode: Text.WordWrap }
                        }
                        
                        // Installed addons list - Grid layout
                        Column {
                            width: parent.width
                            spacing: 10
                            Text { text: "INSTALLED ADDONS"; font.pixelSize: 14; font.bold: true; font.letterSpacing: 0.5; color: "#aaaaaa"; visible: addonListModel.count > 0 }
                            
                            Flow {
                                width: parent.width
                                spacing: 20
                                visible: addonListModel.count > 0
                                
                                Repeater {
                                    model: addonListModel
                                    
                                    Rectangle {
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
                                            
                                            // Addon name
                                            Text {
                                                width: parent.width
                                                text: model.name || model.id || "Unknown Addon"
                                                font.pixelSize: 18
                                                font.bold: true
                                                color: "#FFFFFF"
                                                elide: Text.ElideRight
                                            }
                                            
                                            // Version and status
                                            Row {
                                                spacing: 12
                                                Text { text: "v" + (model.version || "N/A"); font.pixelSize: 12; color: "#aaaaaa" }
                                                Row {
                                                    spacing: 6; anchors.verticalCenter: parent.verticalCenter
                                                    Rectangle { width: 8; height: 8; radius: 4; anchors.verticalCenter: parent.verticalCenter; color: (model.enabled || false) ? "#4CAF50" : "#666666" }
                                                    Text { text: (model.enabled || false) ? "Active" : "Inactive"; font.pixelSize: 12; color: (model.enabled || false) ? "#4CAF50" : "#666666"; anchors.verticalCenter: parent.verticalCenter }
                                                }
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
                                                
                                                // Toggle switch - Standard Qt Switch
                                                Switch {
                                                    id: toggleSwitch
                                                    checked: model.enabled || false
                                                    onToggled: {
                                                        if (checked) {
                                                            addonRepo.enableAddon(model.id)
                                                        } else {
                                                            addonRepo.disableAddon(model.id)
                                                        }
                                                        refreshAddonList()
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
                                                            deleteConfirmDialog.addonName = model.name || model.id || "Unknown"
                                                            deleteConfirmDialog.addonId = model.id
                                                            deleteConfirmDialog.open()
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            // Empty State
                            Rectangle { width: parent.width; height: 200; color: "transparent"; visible: addonListModel.count === 0; Column { anchors.centerIn: parent; spacing: 12; Text { anchors.horizontalCenter: parent.horizontalCenter; text: "No addons installed"; font.pixelSize: 18; font.bold: true; color: "#ffffff" } Text { anchors.horizontalCenter: parent.horizontalCenter; text: "Install addons to extend functionality."; font.pixelSize: 14; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; width: 400; wrapMode: Text.WordWrap } } }
                        }
                    }
                }
                
                // ==========================================
                // ACCOUNTS AND API SECTION
                // ==========================================
                Rectangle {
                    width: parent.width
                    height: accountsSection.height + 40
                    color: "#1a1a1a"
                    radius: 8
                    
                    Column {
                        id: accountsSection
                        width: parent.width - 40
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.margins: 20
                        spacing: 20
                        
                        // Header
                        Column {
                            width: parent.width
                            spacing: 4
                            Text { 
                                text: "Accounts and API"
                                font.pixelSize: 28
                                font.bold: true
                                color: "#ffffff"
                            }
                            Text { 
                                text: "Manage your account connections and API keys."
                                font.pixelSize: 14
                                color: "#aaaaaa"
                                wrapMode: Text.WordWrap
                                width: parent.width
                            }
                        }
                        
                        // Cards in Flow layout
                        Flow {
                            width: parent.width
                            spacing: 20
                            
                            // Trakt Card
                            Rectangle {
                                id: traktCardRectangle
                                width: Math.max(350, Math.min(450, (parent.width - 20) / 2))
                                height: traktCardContent.height + 32
                                color: "#252525"
                                radius: 8
                                border.color: "#2d2d2d"
                                border.width: 1
                                property int pendingSyncs: 0
                                
                                Column {
                                    id: traktCardContent
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.top: parent.top
                                    anchors.margins: 16
                                    spacing: 12
                                    
                                    // Title
                                    Text {
                                        width: parent.width
                                        text: "Trakt"
                                        font.pixelSize: 20
                                        font.bold: true
                                        color: "#FFFFFF"
                                    }
                                    
                                    // Description
                                    Text {
                                        width: parent.width
                                        text: "Sync your watch history with Trakt."
                                        font.pixelSize: 13
                                        color: "#AAAAAA"
                                        wrapMode: Text.WordWrap
                                    }
                                    
                                    // Status
                                    Text {
                                        id: currentUserText
                                        width: parent.width
                                        font.pixelSize: 12
                                        color: "#aaaaaa"
                                        wrapMode: Text.WordWrap
                                        visible: text !== ""
                                    }
                                    
                                    Text {
                                        id: deviceCodeText
                                        width: parent.width
                                        font.pixelSize: 12
                                        color: "#ffffff"
                                        wrapMode: Text.WordWrap
                                        visible: false
                                    }
                                    
                                    Text {
                                        id: traktStatusText
                                        width: parent.width
                                        font.pixelSize: 12
                                        color: "#aaaaaa"
                                        wrapMode: Text.WordWrap
                                    }
                                    
                                    // Buttons row
                                    Row {
                                        width: parent.width
                                        spacing: 10
                                        
                                        Button {
                                            id: loginButton
                                            width: 150
                                            height: 36
                                            text: TraktAuthService.isAuthenticated ? "Logout" : "Login with Trakt"
                                            enabled: TraktAuthService.isConfigured
                                            background: Rectangle { 
                                                color: parent.pressed ? "#e0e0e0" : "#ffffff"
                                                radius: 4
                                            }
                                            contentItem: Text { 
                                                text: parent.text
                                                color: "#000000"
                                                font.bold: true
                                                font.pixelSize: 13
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                            }
                                            onClicked: { 
                                                if (TraktAuthService.isAuthenticated) { 
                                                    TraktAuthService.logout()
                                                    currentUserText.text = ""
                                                } else { 
                                                    TraktAuthService.generateDeviceCode()
                                                }
                                            }
                                        }
                                        
                                        Button {
                                            id: resyncButton
                                            width: 36
                                            height: 36
                                            visible: TraktAuthService.isAuthenticated
                                            enabled: TraktAuthService.isAuthenticated
                                            
                                            background: Rectangle {
                                                color: parent.pressed ? "#22ffffff" : "transparent"
                                                radius: 4
                                            }
                                            
                                            contentItem: Item {
                                                Image {
                                                    source: "qrc:/assets/icons/refresh.svg"
                                                    sourceSize.width: 128
                                                    sourceSize.height: 128
                                                    mipmap: true
                                                    smooth: true
                                                    antialiasing: true
                                                    width: 24
                                                    height: 24
                                                    anchors.centerIn: parent
                                                    fillMode: Image.PreserveAspectFit
                                                }
                                            }
                                            onClicked: { 
                                                traktCardRectangle.pendingSyncs = 2
                                                traktStatusText.text = "Resyncing..."
                                                traktStatusText.color = "#2196F3"
                                                resyncButton.enabled = false
                                                TraktCoreService.resyncWatchedHistory()
                                            }
                                        }
                                    }
                                }
                                
                                Connections {
                                    target: TraktAuthService
                                    function onDeviceCodeGenerated(u, v, e) {
                                        deviceCodeText.text = v + " : " + u
                                        deviceCodeText.visible = true
                                        pollingTimer.start()
                                    }
                                    function onAuthenticationStatusChanged(a) {
                                        if (a) {
                                            deviceCodeText.visible = false
                                            pollingTimer.stop()
                                            TraktAuthService.getCurrentUser()
                                            loginButton.text = "Logout"
                                        } else {
                                            loginButton.text = "Login"
                                            currentUserText.text = ""
                                        }
                                    }
                                    function onUserInfoFetched(u, s) {
                                        currentUserText.text = "Logged in: " + u
                                        currentUserText.color = "#4CAF50"
                                    }
                                    function onError(m) {
                                        traktStatusText.text = "Error: " + m
                                        traktStatusText.color = "#F44336"
                                    }
                                }
                                
                                Connections {
                                    target: TraktCoreService
                                    function onWatchedMoviesSynced(a, u) {
                                        traktStatusText.text = "Movies synced: " + a
                                        traktCardRectangle.pendingSyncs = Math.max(0, traktCardRectangle.pendingSyncs - 1)
                                        if (traktCardRectangle.pendingSyncs === 0) resyncButton.enabled = true
                                    }
                                    function onWatchedShowsSynced(a, u) {
                                        traktStatusText.text = "Shows synced: " + a
                                        traktCardRectangle.pendingSyncs = Math.max(0, traktCardRectangle.pendingSyncs - 1)
                                        if (traktCardRectangle.pendingSyncs === 0) resyncButton.enabled = true
                                    }
                                    function onSyncError(t, m) {
                                        traktStatusText.text = "Error: " + m
                                        traktCardRectangle.pendingSyncs = Math.max(0, traktCardRectangle.pendingSyncs - 1)
                                        if (traktCardRectangle.pendingSyncs === 0) resyncButton.enabled = true
                                    }
                                }
                                
                                Timer {
                                    id: pollingTimer
                                    interval: 5000
                                    repeat: true
                                    running: false
                                    onTriggered: {
                                        if (TraktAuthService.isAuthenticated) stop()
                                        else TraktAuthService.checkAuthentication()
                                    }
                                }
                                
                                Component.onCompleted: {
                                    TraktAuthService.checkAuthentication()
                                }
                            }
                            
                            // OMDB API Key Card
                            Rectangle {
                                width: Math.max(350, Math.min(450, (parent.width - 20) / 2))
                                height: omdbCardContent.height + 32
                                color: "#252525"
                                radius: 8
                                border.color: "#2d2d2d"
                                border.width: 1
                                
                                Column {
                                    id: omdbCardContent
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.top: parent.top
                                    anchors.margins: 16
                                    spacing: 12
                                    
                                    // Title
                                    Text {
                                        width: parent.width
                                        text: "OMDB API Key"
                                        font.pixelSize: 20
                                        font.bold: true
                                        color: "#FFFFFF"
                                    }
                                    
                                    // Description
                                    Text {
                                        width: parent.width
                                        text: "Add your OMDB API key to enable extra ratings (Rotten Tomatoes, Metacritic, etc.)"
                                        font.pixelSize: 13
                                        color: "#AAAAAA"
                                        wrapMode: Text.WordWrap
                                    }
                                    
                                    // Status
                                    Text {
                                        id: omdbStatusText
                                        width: parent.width
                                        font.pixelSize: 12
                                        color: configuration.omdbApiKey ? "#4CAF50" : "#aaaaaa"
                                        text: configuration.omdbApiKey ? "API key configured" : "No API key set"
                                        wrapMode: Text.WordWrap
                                    }
                                    
                                    // Input field
                                    Rectangle {
                                        width: parent.width
                                        height: 44
                                        color: "#2d2d2d"
                                        border.width: 1
                                        border.color: omdbKeyField.activeFocus ? "#ffffff" : "#3d3d3d"
                                        radius: 4
                                        
                                        TextField {
                                            id: omdbKeyField
                                            anchors.fill: parent
                                            anchors.margins: 8
                                            placeholderText: "Enter OMDB API key"
                                            color: "#ffffff"
                                            placeholderTextColor: "#666666"
                                            background: Rectangle { color: "transparent" }
                                            text: configuration.omdbApiKey || ""
                                            echoMode: TextInput.Password
                                        }
                                    }
                                    
                                    // Save button
                                    Button {
                                        width: 120
                                        height: 36
                                        text: "Save"
                                        background: Rectangle {
                                            color: parent.pressed ? "#e0e0e0" : "#ffffff"
                                            radius: 4
                                        }
                                        contentItem: Text {
                                            text: parent.text
                                            color: "#000000"
                                            font.bold: true
                                            font.pixelSize: 13
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                        }
                                        onClicked: {
                                            if (omdbKeyField.text.trim() !== "") {
                                                if (configuration.saveOmdbApiKey(omdbKeyField.text.trim())) {
                                                    omdbStatusText.text = "API key saved successfully"
                                                    omdbStatusText.color = "#4CAF50"
                                                } else {
                                                    omdbStatusText.text = "Failed to save API key"
                                                    omdbStatusText.color = "#F44336"
                                                }
                                            } else {
                                                // Clear the key
                                                if (configuration.saveOmdbApiKey("")) {
                                                    omdbStatusText.text = "API key cleared"
                                                    omdbStatusText.color = "#aaaaaa"
                                                }
                                            }
                                        }
                                    }
                                }
                                
                                Connections {
                                    target: configuration
                                    function onOmdbApiKeyChanged() {
                                        omdbKeyField.text = configuration.omdbApiKey || ""
                                        omdbStatusText.text = configuration.omdbApiKey ? "API key configured" : "No API key set"
                                        omdbStatusText.color = configuration.omdbApiKey ? "#4CAF50" : "#aaaaaa"
                                    }
                                }
                            }
                        }
                    }
                }
                
                // ==========================================
                // CATALOG SECTION
                // ==========================================
                Rectangle {
                    width: parent.width
                    height: catalogSection.height + 40
                    color: "#1a1a1a"
                    radius: 8
                    
                    Column {
                        id: catalogSection
                        width: parent.width - 40; anchors.left: parent.left; anchors.top: parent.top; anchors.margins: 20; spacing: 20
                        
                        Row {
                            width: parent.width; spacing: 20
                            Column {
                                width: parent.width - refreshCatalogButton.width - parent.spacing; spacing: 4
                                Text { text: "Catalog Management"; font.pixelSize: 24; font.bold: true; color: "#ffffff" }
                                Text { text: "Manage content catalogs."; font.pixelSize: 14; color: "#aaaaaa"; wrapMode: Text.WordWrap; width: parent.width }
                            }
                            
                            // --- REFRESH CATALOG BUTTON ---
                            Button {
                                id: refreshCatalogButton
                                width: 44
                                height: 44

                                // 1. Transparent Background
                                background: Rectangle {
                                    color: parent.pressed ? "#22ffffff" : "transparent"
                                    radius: 4
                                }

                                contentItem: Item {
                                    Image {
                                        id: refreshCatalogIcon
                                        source: "qrc:/assets/icons/refresh.svg"
                                        
                                        // 2. High Quality Settings
                                        sourceSize.width: 128
                                        sourceSize.height: 128
                                        mipmap: true
                                        smooth: true
                                        antialiasing: true

                                        width: 36
                                        height: 36
                                        anchors.centerIn: parent
                                        fillMode: Image.PreserveAspectFit
                                    }
                                }
                                onClicked: { refreshCatalogList() }
                            }
                        }
                        
                        Connections { target: catalogPrefsService; function onCatalogsUpdated(){ refreshCatalogList() } function onError(m){ catalogStatusText.text="Error: "+m; catalogStatusText.color="#F44336" } }
                        Text { id: catalogStatusText; width: parent.width; font.pixelSize: 14; color: "#aaaaaa"; wrapMode: Text.WordWrap }
                        Column {
                            width: parent.width; spacing: 10; visible: catalogListModel.count > 0
                            Text { text: "AVAILABLE CATALOGS"; font.pixelSize: 14; font.bold: true; font.letterSpacing: 0.5; color: "#aaaaaa" }
                            
                            ListView {
                                id: catalogListView
                                width: parent.width; height: Math.min(catalogListModel.count * 100, 500)
                                model: catalogListModel; clip: true; spacing: 12
                                move: Transition { NumberAnimation { properties: "x,y"; duration: 300; easing.type: Easing.OutCubic } }

                                delegate: Item {
                                    id: delegateWrapper; width: ListView.view.width; height: 95; z: dragArea.pressed ? 100 : 1 
                                    Rectangle {
                                        id: visualCard
                                        width: parent.width; height: parent.height
                                        color: dragArea.pressed ? "#252525" : "#1a1a1a"; radius: 8; border.width: 1; border.color: dragArea.pressed ? "#4d4d4d" : "#2d2d2d"; scale: dragArea.pressed ? 1.02 : 1.0
                                        anchors.fill: dragArea.pressed ? undefined : parent
                                        Behavior on scale { NumberAnimation { duration: 150 } }
                                        Behavior on color { ColorAnimation { duration: 150 } }
                                        
                                        Row {
                                            anchors.fill: parent; anchors.margins: 16; spacing: 12
                                            // Drag Handle
                                            Item {
                                                width: 30; height: parent.height; anchors.verticalCenter: parent.verticalCenter
                                                Text { anchors.centerIn: parent; text: "\u2630"; color: dragArea.pressed ? "#ffffff" : "#666666"; font.pixelSize: 24; font.bold: true }
                                                MouseArea {
                                                    id: dragArea; anchors.fill: parent; cursorShape: Qt.OpenHandCursor
                                                    drag.target: visualCard; drag.axis: Drag.YAxis; drag.smoothed: true
                                                    property int pendingIndex: -1
                                                    Timer { id: reorderTimer; interval: 150; repeat: false; onTriggered: { if (dragArea.pendingIndex !== -1 && dragArea.pendingIndex !== index) { catalogListModel.move(index, dragArea.pendingIndex, 1); dragArea.pendingIndex = -1 } } }
                                                    onPressed: (mouse) => { var pos=visualCard.mapToItem(root,0,0); visualCard.width=delegateWrapper.width; visualCard.height=delegateWrapper.height; visualCard.parent=root; visualCard.x=pos.x; visualCard.y=pos.y; visualCard.z=1000; catalogListView.interactive=false }
                                                    onReleased: (mouse) => { visualCard.parent=delegateWrapper; visualCard.x=0; visualCard.y=0; visualCard.z=1; visualCard.anchors.fill=delegateWrapper; catalogListView.interactive=true; reorderTimer.stop(); dragArea.pendingIndex=-1 }
                                                    onPositionChanged: (mouse) => { var rootCenter=Qt.point(visualCard.x+visualCard.width/2, visualCard.y+visualCard.height/2); var contentPos=catalogListView.contentItem.mapFromItem(root, rootCenter.x, rootCenter.y); var indexUnderMouse=catalogListView.indexAt(contentPos.x, contentPos.y); if(indexUnderMouse!==-1 && indexUnderMouse!==index){ if(dragArea.pendingIndex!==indexUnderMouse){ dragArea.pendingIndex=indexUnderMouse; reorderTimer.restart() } }else{ reorderTimer.stop(); dragArea.pendingIndex=-1 } }
                                                }
                                            }
                                            // Content
                                            Column {
                                                width: parent.width - 30 - enableSwitch.width - 40 - 30; anchors.verticalCenter: parent.verticalCenter
                                                Text { width: parent.width; text: model.catalogName || "Unknown"; font.pixelSize: 16; font.bold: true; color: "#ffffff"; elide: Text.ElideRight }
                                                Text { width: parent.width; text: model.addonName + " \u2022 " + (model.catalogType||""); font.pixelSize: 12; color: "#aaaaaa"; elide: Text.ElideRight }
                                                Text { width: parent.width; text: "ID: "+model.catalogId; font.pixelSize: 11; color: "#666666"; elide: Text.ElideRight; visible: model.catalogId }
                                            }
                                            Button {
                                                id: heroToggle; width: 80; height: 36; anchors.verticalCenter: parent.verticalCenter; text: model.isHeroSource ? "Hero \u2713" : "Hero"
                                                background: Rectangle { color: model.isHeroSource ? "#ffffff" : "#2d2d2d"; radius: 4; border.width: 1; border.color: model.isHeroSource ? "#ffffff" : "#3d3d3d" }
                                                contentItem: Text { text: parent.text; color: model.isHeroSource ? "#000000" : "#ffffff"; font.bold: true; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                                onClicked: { if(model.isHeroSource) catalogPrefsService.unsetHeroCatalog(model.addonId, model.catalogType, model.catalogId||""); else catalogPrefsService.setHeroCatalog(model.addonId, model.catalogType, model.catalogId||"") }
                                            }
                                            Switch { id: enableSwitch; anchors.verticalCenter: parent.verticalCenter; checked: model.enabled || false; onToggled: { catalogPrefsService.toggleCatalogEnabled(model.addonId, model.catalogType, model.catalogId||"", checked) } }
                                        }
                                    }
                                }
                            }
                        }
                        // Empty State
                        Rectangle { width: parent.width; height: 200; color: "transparent"; visible: catalogListModel.count===0; Column { anchors.centerIn: parent; spacing: 12; Text { anchors.horizontalCenter: parent.horizontalCenter; text: "No catalogs"; font.pixelSize: 18; font.bold: true; color: "#ffffff" } } }
                    }
                }
            }
        }
    }
    
    // Models / Logic
    ListModel { id: addonListModel }
    ListModel { id: catalogListModel }
    function refreshAddonList() { addonListModel.clear(); let addons = addonRepo.getAllAddons(); for(let i=0; i<addons.length; i++) addonListModel.append({id: addons[i].id, name: addons[i].name, version: addons[i].version, description: addons[i].description || "", enabled: addons[i].enabled}); addonStatusText.text="Total: "+addonListModel.count }
    function refreshCatalogList() { catalogListModel.clear(); let catalogs = catalogPrefsService.getAvailableCatalogs(); for(let i=0; i<catalogs.length; i++) catalogListModel.append({addonId: catalogs[i].addonId, addonName: catalogs[i].addonName, catalogType: catalogs[i].catalogType, catalogId: catalogs[i].catalogId, catalogName: catalogs[i].catalogName, enabled: catalogs[i].enabled, isHeroSource: catalogs[i].isHeroSource}); catalogStatusText.text="Total: "+catalogListModel.count }
    
    // Delete Dialog
    MessageDialog { id: deleteConfirmDialog; property string addonName; property string addonId; title: "Remove Addon"; text: "Remove \""+addonName+"\"?"; buttons: MessageDialog.Ok|MessageDialog.Cancel; onAccepted: if(addonId) addonRepo.removeAddon(addonId) }
    Component.onCompleted: { refreshAddonList(); refreshCatalogList() }
}