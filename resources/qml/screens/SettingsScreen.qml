import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Yantrium.Components 1.0
import Yantrium.Services 1.0

Item {
    id: root
    anchors.fill: parent
    
    // Services (singletons, accessed directly)
    property AddonRepository addonRepo: AddonRepository
    property CatalogPreferencesService catalogPrefsService: CatalogPreferencesService
    
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
                        
                        // Header with title, description, and badge
                        Item {
                            width: parent.width
                            height: Math.max(headerColumn.height, badge.height)
                            
                            Column {
                                id: headerColumn
                                width: parent.width - (badge.visible ? badge.width + 20 : 0)
                                
                                Text {
                                    text: "Addons"
                                    font.pixelSize: 28
                                    font.bold: true
                                    color: "#ffffff"
                                }
                                
                                Text {
                                    text: "Manage your installed extensions and sources."
                                    font.pixelSize: 14
                                    color: "#aaaaaa"
                                    wrapMode: Text.WordWrap
                                    width: parent.width
                                }
                            }
                            
                            Rectangle {
                                id: badge
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.topMargin: 4
                                width: badgeText.width + 16
                                height: 24
                                radius: 12
                                color: "#2d2d2d"
                                visible: addonListModel.count > 0
                                
                                Text {
                                    id: badgeText
                                    anchors.centerIn: parent
                                    text: addonListModel.count + " Installed"
                                    font.pixelSize: 12
                                    font.bold: true
                                    color: "#aaaaaa"
                                }
                            }
                        }
                        
                        Connections {
                            target: addonRepo
                            function onAddonInstalled(addon) {
                                addonStatusText.text = "\u2713 Addon installed: " + addon.name
                                addonStatusText.color = "#4CAF50"
                                refreshAddonList()
                            }
                            
                            function onAddonRemoved(addonId) {
                                addonStatusText.text = "\u2713 Removed addon: " + addonId
                                addonStatusText.color = "#FF9800"
                                refreshAddonList()
                            }
                            
                            function onError(errorMessage) {
                                addonStatusText.text = "\u2717 Error: " + errorMessage
                                addonStatusText.color = "#F44336"
                            }
                        }
                        
                        // Add new addon
                        Column {
                            width: parent.width
                            spacing: 10
                            
                            Text {
                                text: "Install Addon"
                                font.pixelSize: 16
                                font.bold: true
                                color: "#ffffff"
                            }
                            
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
                                        anchors.left: parent.left
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.leftMargin: 12
                                        anchors.rightMargin: 12
                                        spacing: 8
                                        
                                        Text {
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: "\U0001f517"
                                            font.pixelSize: 16
                                            color: "#666666"
                                        }
                                        
                                        TextField {
                                            id: manifestUrlField
                                            width: parent.parent.width - 40
                                            anchors.verticalCenter: parent.verticalCenter
                                            placeholderText: "Enter addon manifest URL (e.g., https://...)"
                                            color: "#ffffff"
                                            placeholderTextColor: "#666666"
                                            background: Rectangle {
                                                color: "transparent"
                                            }
                                        }
                                    }
                                }
                                
                                Button {
                                    id: installButton
                                    width: 120
                                    height: 44
                                    text: "Install"
                                    
                                    background: Rectangle {
                                        color: parent.pressed ? "#e0e0e0" : "#ffffff"
                                        radius: 4
                                    }
                                    
                                    contentItem: Text {
                                        text: parent.text
                                        color: "#000000"
                                        font.bold: true
                                        font.pixelSize: 14
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    
                                    onClicked: {
                                        if (manifestUrlField.text.trim() !== "") {
                                            addonStatusText.text = "Installing..."
                                            addonStatusText.color = "#2196F3"
                                            addonRepo.installAddon(manifestUrlField.text.trim())
                                            manifestUrlField.text = ""
                                        }
                                    }
                                }
                            }
                            
                            Text {
                                id: addonStatusText
                                width: parent.width
                                font.pixelSize: 14
                                color: "#aaaaaa"
                                wrapMode: Text.WordWrap
                            }
                        }
                        
                        // Installed addons list
                        Column {
                            width: parent.width
                            spacing: 10
                            
                            Text {
                                text: "INSTALLED ADDONS"
                                font.pixelSize: 14
                                font.bold: true
                                font.letterSpacing: 0.5
                                color: "#aaaaaa"
                                visible: addonListModel.count > 0
                            }
                            
                            ListView {
                                width: parent.width
                                height: Math.min(addonListModel.count * 90, 500)
                                model: addonListModel
                                clip: true
                                spacing: 12
                                visible: addonListModel.count > 0
                                
                                delegate: Rectangle {
                                    width: ListView.view.width
                                    height: 85
                                    color: "#1a1a1a"
                                    radius: 8
                                    border.width: 1
                                    border.color: "#2d2d2d"
                                    
                                    Row {
                                        anchors.fill: parent
                                        anchors.margins: 16
                                        spacing: 16
                                        
                                        Column {
                                            width: parent.width - toggleSwitch.width - deleteButton.width - 40
                                            anchors.verticalCenter: parent.verticalCenter
                                            spacing: 8
                                            
                                            Text {
                                                width: parent.width
                                                text: model.name || model.id || "Unknown"
                                                font.pixelSize: 16
                                                font.bold: true
                                                color: "#ffffff"
                                                elide: Text.ElideRight
                                            }
                                            
                                            Row {
                                                spacing: 12
                                                
                                                Text {
                                                    text: "v" + (model.version || "N/A")
                                                    font.pixelSize: 12
                                                    color: "#aaaaaa"
                                                }
                                                
                                                // Status indicator
                                                Row {
                                                    spacing: 6
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    
                                                    Rectangle {
                                                        width: 8
                                                        height: 8
                                                        radius: 4
                                                        anchors.verticalCenter: parent.verticalCenter
                                                        color: (model.enabled || false) ? "#4CAF50" : "#666666"
                                                    }
                                                    
                                                    Text {
                                                        text: (model.enabled || false) ? "Active" : "Inactive"
                                                        font.pixelSize: 12
                                                        color: (model.enabled || false) ? "#4CAF50" : "#666666"
                                                        anchors.verticalCenter: parent.verticalCenter
                                                    }
                                                }
                                            }
                                        }
                                        
                                        Switch {
                                            id: toggleSwitch
                                            anchors.verticalCenter: parent.verticalCenter
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
                                        
                                        Button {
                                            id: deleteButton
                                            width: 40
                                            height: 40
                                            anchors.verticalCenter: parent.verticalCenter
                                            
                                            background: Rectangle {
                                                color: parent.pressed ? "#d32f2f" : "transparent"
                                                radius: 4
                                            }
                                            
                                            contentItem: Text {
                                                text: "\U0001f5d1\ufe0f"
                                                font.pixelSize: 18
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                            }
                                            
                                            onClicked: {
                                                deleteConfirmDialog.addonName = model.name || model.id || "Unknown"
                                                deleteConfirmDialog.addonId = model.id
                                                deleteConfirmDialog.open()
                                            }
                                        }
                                    }
                                }
                            }
                            
                            // Empty state
                            Rectangle {
                                width: parent.width
                                height: 200
                                color: "transparent"
                                visible: addonListModel.count === 0
                                
                                Column {
                                    anchors.centerIn: parent
                                    spacing: 12
                                    
                                    Text {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: "No addons installed"
                                        font.pixelSize: 18
                                        font.bold: true
                                        color: "#ffffff"
                                    }
                                    
                                    Text {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: "Install addons to extend functionality and add content sources."
                                        font.pixelSize: 14
                                        color: "#aaaaaa"
                                        horizontalAlignment: Text.AlignHCenter
                                        width: 400
                                        wrapMode: Text.WordWrap
                                    }
                                }
                            }
                        }
                    }
                }
                
                // ==========================================
                // TRAKT AUTHENTICATION SECTION
                // ==========================================
                Rectangle {
                    width: parent.width
                    height: traktSection.height + 40
                    color: "#1a1a1a"
                    radius: 8
                    
                    Column {
                        id: traktSection
                        width: parent.width - 40
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.margins: 20
                        spacing: 20
                        
                        property int pendingSyncs: 0
                        
                        function startResync() {
                            pendingSyncs = 2  // Movies and shows
                            traktStatusText.text = "Resyncing watch history... This may take a while."
                            traktStatusText.color = "#2196F3"
                            resyncButton.enabled = false
                            TraktCoreService.resyncWatchedHistory()
                        }
                        
                        Column {
                            width: parent.width
                            spacing: 4
                            
                            Text {
                                text: "Trakt Authentication"
                                font.pixelSize: 24
                                font.bold: true
                                color: "#ffffff"
                            }
                            
                            Text {
                                text: "Sync your watch history and manage your Trakt account."
                                font.pixelSize: 14
                                color: "#aaaaaa"
                                wrapMode: Text.WordWrap
                                width: parent.width
                            }
                        }
                        
                        // TraktAuthService is a singleton, accessed directly
                        Connections {
                            target: TraktAuthService
                            
                            function onDeviceCodeGenerated(userCode, verificationUrl, expiresIn) {
                                deviceCodeText.text = "Go to: " + verificationUrl + "\nEnter code: " + userCode
                                deviceCodeText.visible = true
                                loginButton.enabled = false
                                pollingTimer.start()
                            }
                            
                            function onAuthenticationStatusChanged(authenticated) {
                                if (authenticated) {
                                    deviceCodeText.visible = false
                                    pollingTimer.stop()
                                    TraktAuthService.getCurrentUser()
                                    loginButton.text = "Logout"
                                } else {
                                    loginButton.text = "Login with Trakt"
                                    currentUserText.text = ""
                                }
                            }
                            
                            function onUserInfoFetched(username, slug) {
                                currentUserText.text = "Logged in as: " + username
                                currentUserText.color = "#4CAF50"
                            }
                            
                            function onError(message) {
                                traktStatusText.text = "\u2717 Error: " + message
                                traktStatusText.color = "#F44336"
                                deviceCodeText.visible = false
                                pollingTimer.stop()
                                loginButton.enabled = true
                            }
                        }
                        
                        Component.onCompleted: {
                            TraktAuthService.checkAuthentication()
                        }
                        
                        // Current user status
                        Text {
                            id: currentUserText
                            width: parent.width
                            font.pixelSize: 14
                            color: "#aaaaaa"
                            wrapMode: Text.WordWrap
                        }
                        
                        // Login/Logout button
                        Button {
                            id: loginButton
                            width: 200
                            height: 44
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
                                font.pixelSize: 14
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
                        
                        // Device code display
                        Text {
                            id: deviceCodeText
                            width: parent.width
                            font.pixelSize: 14
                            color: "#ffffff"
                            wrapMode: Text.WordWrap
                            visible: false
                        }
                        
                        // Status text
                        Text {
                            id: traktStatusText
                            width: parent.width
                            font.pixelSize: 14
                            color: "#aaaaaa"
                            wrapMode: Text.WordWrap
                        }
                        
                        // Resync button (only show when authenticated)
                        Button {
                            id: resyncButton
                            width: 200
                            height: 44
                            text: "Resync Watch History"
                            visible: TraktAuthService.isAuthenticated
                            enabled: TraktAuthService.isAuthenticated
                            
                            background: Rectangle {
                                color: parent.pressed ? "#e68900" : "#ff9800"
                                radius: 4
                            }
                            
                            contentItem: Text {
                                text: parent.text
                                color: "#000000"
                                font.bold: true
                                font.pixelSize: 14
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            
                            onClicked: {
                                traktSection.startResync()
                            }
                        }
                        
                        // Connections for resync status
                        Connections {
                            target: TraktCoreService
                            
                            function onWatchedMoviesSynced(addedCount, updatedCount) {
                                traktStatusText.text = "Movies synced: " + addedCount + " items added"
                                traktStatusText.color = "#4CAF50"
                                traktSection.pendingSyncs = Math.max(0, traktSection.pendingSyncs - 1)
                                
                                // If shows are still pending, update message
                                if (traktSection.pendingSyncs > 0) {
                                    traktStatusText.text = "Movies synced: " + addedCount + " items. Waiting for shows..."
                                } else {
                                    // Both complete
                                    resyncButton.enabled = true
                                }
                            }
                            
                            function onWatchedShowsSynced(addedCount, updatedCount) {
                                traktStatusText.text = "Shows synced: " + addedCount + " episodes added. Resync complete!"
                                traktStatusText.color = "#4CAF50"
                                traktSection.pendingSyncs = Math.max(0, traktSection.pendingSyncs - 1)
                                resyncButton.enabled = true
                            }
                            
                            function onSyncError(syncType, message) {
                                traktStatusText.text = "\u2717 Sync error (" + syncType + "): " + message
                                traktStatusText.color = "#F44336"
                                traktSection.pendingSyncs = Math.max(0, traktSection.pendingSyncs - 1)
                                
                                // Re-enable button when all syncs are done (or errored)
                                if (traktSection.pendingSyncs === 0) {
                                    resyncButton.enabled = true
                                }
                            }
                        }
                        
                        // Polling timer for device code
                        Timer {
                            id: pollingTimer
                            interval: 5000  // Poll every 5 seconds
                            repeat: true
                            running: false
                            
                            onTriggered: {
                                if (TraktAuthService.isAuthenticated) {
                                    stop()
                                } else {
                                    // The service handles polling internally, but we can check status
                                    TraktAuthService.checkAuthentication()
                                }
                            }
                        }
                    }
                }
                
                // ==========================================
                // CATALOG MANAGEMENT SECTION
                // ==========================================
                Rectangle {
                    width: parent.width
                    height: catalogSection.height + 40
                    color: "#1a1a1a"
                    radius: 8
                    
                    Column {
                        id: catalogSection
                        width: parent.width - 40
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.margins: 20
                        spacing: 20
                        
                        Column {
                            width: parent.width
                            spacing: 4
                            
                            Text {
                                text: "Catalog Management"
                                font.pixelSize: 24
                                font.bold: true
                                color: "#ffffff"
                            }
                            
                            Text {
                                text: "Manage content catalogs from your installed addons."
                                font.pixelSize: 14
                                color: "#aaaaaa"
                                wrapMode: Text.WordWrap
                                width: parent.width
                            }
                        }
                        
                        Connections {
                            target: catalogPrefsService
                            function onCatalogsUpdated() {
                                refreshCatalogList()
                            }
                            
                            function onError(message) {
                                catalogStatusText.text = "\u2717 Error: " + message
                                catalogStatusText.color = "#F44336"
                            }
                        }
                        
                        Row {
                            spacing: 10
                            
                            Button {
                                width: 180
                                height: 44
                                text: "Refresh Catalogs"
                                
                                background: Rectangle {
                                    color: parent.pressed ? "#ffffff" : "#ffffff"
                                    radius: 4
                                }
                                
                                contentItem: Text {
                                    text: parent.text
                                    color: "#000000"
                                    font.bold: true
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                
                                onClicked: {
                                    refreshCatalogList()
                                }
                            }
                            
                            Button {
                                width: 180
                                height: 44
                                text: "Export (Processed)"
                                
                                background: Rectangle {
                                    color: parent.pressed ? "#ffffff" : "#ffffff"
                                    radius: 4
                                }
                                
                                contentItem: Text {
                                    text: parent.text
                                    color: "#000000"
                                    font.bold: true
                                    font.pixelSize: 12
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                
                                onClicked: {
                                    // Try to get existing catalog sections first
                                    let existingSections = libraryService.getCatalogSections()
                                    
                                    if (existingSections && existingSections.length > 0) {
                                        exportCatalogDataToFile(existingSections, false)
                                    } else {
                                        catalogStatusText.text = "Loading catalogs..."
                                        catalogStatusText.color = "#2196F3"
                                        exportCatalogData()
                                    }
                                }
                            }
                            
                            Button {
                                width: 180
                                height: 44
                                text: "Export (Raw)"
                                
                                background: Rectangle {
                                    color: parent.pressed ? "#ffffff" : "#ffffff"
                                    radius: 4
                                }
                                
                                contentItem: Text {
                                    text: parent.text
                                    color: "#000000"
                                    font.bold: true
                                    font.pixelSize: 12
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                
                                onClicked: {
                                    catalogStatusText.text = "Loading raw catalog data..."
                                    catalogStatusText.color = "#2196F3"
                                    exportTimeout.start()
                                    libraryService.loadCatalogsRaw()
                                }
                            }
                        }
                        
                        Text {
                            id: catalogStatusText
                            width: parent.width
                            font.pixelSize: 14
                            color: "#aaaaaa"
                            wrapMode: Text.WordWrap
                        }
                        
                        // Catalog list
                        Column {
                            width: parent.width
                            spacing: 10
                            visible: catalogListModel.count > 0
                            
                            Text {
                                text: "AVAILABLE CATALOGS"
                                font.pixelSize: 14
                                font.bold: true
                                font.letterSpacing: 0.5
                                color: "#aaaaaa"
                            }
                            
                            // Updated ListView with "Ghost" Reparenting Strategy + Fixed Sizing
                            ListView {
                                id: catalogListView
                                width: parent.width
                                height: Math.min(catalogListModel.count * 100, 500)
                                model: catalogListModel
                                clip: true
                                spacing: 12
                                
                                // Smooth animation for items moving OUT of the way
                                move: Transition {
                                    NumberAnimation { properties: "x,y"; duration: 300; easing.type: Easing.OutCubic }
                                }

                                delegate: Item {
                                    id: delegateWrapper
                                    width: ListView.view.width
                                    height: 95
                                    z: dragArea.pressed ? 100 : 1 
                                    
                                    // The actual visible card
                                    Rectangle {
                                        id: visualCard
                                        // These bindings are fine normally, but we break them during drag
                                        width: parent.width
                                        height: parent.height
                                        
                                        color: dragArea.pressed ? "#252525" : "#1a1a1a"
                                        radius: 8
                                        border.width: 1
                                        border.color: dragArea.pressed ? "#4d4d4d" : "#2d2d2d"
                                        scale: dragArea.pressed ? 1.02 : 1.0
                                        
                                        // Anchors normally fill wrapper, but we break this when dragging
                                        anchors.fill: dragArea.pressed ? undefined : parent

                                        Behavior on scale { NumberAnimation { duration: 150 } }
                                        Behavior on color { ColorAnimation { duration: 150 } }

                                        Row {
                                            anchors.fill: parent
                                            anchors.margins: 16
                                            spacing: 12

                                            // --- DRAG HANDLE ---
                                            Item {
                                                width: 30
                                                height: parent.height
                                                anchors.verticalCenter: parent.verticalCenter
                                                
                                                Text {
                                                    anchors.centerIn: parent
                                                    text: "\u2630"
                                                    color: dragArea.pressed ? "#ffffff" : "#666666"
                                                    font.pixelSize: 24
                                                    font.bold: true
                                                }

                                                MouseArea {
                                                    id: dragArea
                                                    anchors.fill: parent
                                                    cursorShape: Qt.OpenHandCursor
                                                    
                                                    drag.target: visualCard
                                                    drag.axis: Drag.YAxis
                                                    drag.smoothed: true
                                                    
                                                    property int pendingIndex: -1

                                                    // Debounce Timer
                                                    Timer {
                                                        id: reorderTimer
                                                        interval: 150 
                                                        repeat: false
                                                        onTriggered: {
                                                            if (dragArea.pendingIndex !== -1 && dragArea.pendingIndex !== index) {
                                                                catalogListModel.move(index, dragArea.pendingIndex, 1)
                                                                dragArea.pendingIndex = -1
                                                            }
                                                        }
                                                    }

                                                    onPressed: (mouse) => {
                                                        // 1. Calculate global position
                                                        var pos = visualCard.mapToItem(root, 0, 0)
                                                        
                                                        // 2. CRITICAL FIX: Lock dimensions to exact pixels before reparenting
                                                        // Prevents inheriting root's full height
                                                        visualCard.width = delegateWrapper.width
                                                        visualCard.height = delegateWrapper.height
                                                        
                                                        // 3. Detach from list: Reparent to root so it floats freely
                                                        visualCard.parent = root
                                                        visualCard.x = pos.x
                                                        visualCard.y = pos.y
                                                        visualCard.z = 1000 // Force to very top
                                                        
                                                        // Stop list interaction
                                                        catalogListView.interactive = false
                                                    }

                                                    onReleased: (mouse) => {
                                                        // 1. Re-attach to the list wrapper
                                                        visualCard.parent = delegateWrapper
                                                        visualCard.x = 0
                                                        visualCard.y = 0
                                                        visualCard.z = 1
                                                        
                                                        // 2. Restore anchors (implicitly restores size bindings too)
                                                        visualCard.anchors.fill = delegateWrapper

                                                        catalogListView.interactive = true
                                                        reorderTimer.stop()
                                                        dragArea.pendingIndex = -1
                                                    }

                                                    onPositionChanged: (mouse) => {
                                                        // Calculate center of the dragging card in CONTENT coordinates
                                                        var rootCenter = Qt.point(visualCard.x + visualCard.width/2, visualCard.y + visualCard.height/2)
                                                        var contentPos = catalogListView.contentItem.mapFromItem(root, rootCenter.x, rootCenter.y)
                                                        
                                                        // Find index under that center
                                                        var indexUnderMouse = catalogListView.indexAt(contentPos.x, contentPos.y)
                                                        
                                                        if (indexUnderMouse !== -1 && indexUnderMouse !== index) {
                                                            if (dragArea.pendingIndex !== indexUnderMouse) {
                                                                dragArea.pendingIndex = indexUnderMouse
                                                                reorderTimer.restart()
                                                            }
                                                        } else {
                                                            reorderTimer.stop()
                                                            dragArea.pendingIndex = -1
                                                        }
                                                    }
                                                }
                                            }
                                            // -------------------

                                            Column {
                                                width: parent.width - 30 - enableSwitch.width - 40 - 30 
                                                anchors.verticalCenter: parent.verticalCenter
                                                
                                                Text {
                                                    width: parent.width
                                                    text: model.catalogName || "Unknown Catalog"
                                                    font.pixelSize: 16
                                                    font.bold: true
                                                    color: "#ffffff"
                                                    elide: Text.ElideRight
                                                }
                                                
                                                Text {
                                                    width: parent.width
                                                    text: model.addonName + " \u2022 " + (model.catalogType || "")
                                                    font.pixelSize: 12
                                                    color: "#aaaaaa"
                                                    elide: Text.ElideRight
                                                }
                                                
                                                Text {
                                                    width: parent.width
                                                    text: "ID: " + model.catalogId
                                                    font.pixelSize: 11
                                                    color: "#666666"
                                                    elide: Text.ElideRight
                                                    visible: model.catalogId && model.catalogId !== ""
                                                }
                                            }
                                            
                                            Button {
                                                id: heroToggle
                                                width: 80
                                                height: 36
                                                anchors.verticalCenter: parent.verticalCenter
                                                text: model.isHeroSource ? "Hero \u2713" : "Hero"
                                                
                                                background: Rectangle {
                                                    color: model.isHeroSource 
                                                        ? (parent.pressed ? "#ffffff" : "#ffffff")
                                                        : (parent.pressed ? "#3d3d3d" : "#2d2d2d")
                                                    radius: 4
                                                    border.width: 1
                                                    border.color: model.isHeroSource ? "#ffffff" : "#3d3d3d"
                                                }
                                                
                                                contentItem: Text {
                                                    text: parent.text
                                                    color: model.isHeroSource ? "#000000" : "#ffffff"
                                                    font.bold: true
                                                    font.pixelSize: 12
                                                    horizontalAlignment: Text.AlignHCenter
                                                    verticalAlignment: Text.AlignVCenter
                                                }
                                                
                                                onClicked: {
                                                    if (model.isHeroSource) {
                                                        catalogPrefsService.unsetHeroCatalog(
                                                            model.addonId, 
                                                            model.catalogType, 
                                                            model.catalogId || ""
                                                        )
                                                    } else {
                                                        catalogPrefsService.setHeroCatalog(
                                                            model.addonId, 
                                                            model.catalogType, 
                                                            model.catalogId || ""
                                                        )
                                                    }
                                                }
                                            }
                                            
                                            Switch {
                                                id: enableSwitch
                                                anchors.verticalCenter: parent.verticalCenter
                                                checked: model.enabled || false
                                                
                                                onToggled: {
                                                    catalogPrefsService.toggleCatalogEnabled(
                                                        model.addonId,
                                                        model.catalogType,
                                                        model.catalogId || "",
                                                        checked
                                                    )
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        
                        // Empty state
                        Rectangle {
                            width: parent.width
                            height: 200
                            color: "transparent"
                            visible: catalogListModel.count === 0
                            
                            Column {
                                anchors.centerIn: parent
                                spacing: 12
                                
                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "No catalogs available"
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "#ffffff"
                                }
                                
                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "Enable addons with catalog support to see catalogs here."
                                    font.pixelSize: 14
                                    color: "#aaaaaa"
                                    horizontalAlignment: Text.AlignHCenter
                                    width: 400
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Addon list model
    ListModel {
        id: addonListModel
    }
    
    // Catalog list model
    ListModel {
        id: catalogListModel
    }
    
    // Function to refresh addon list
    function refreshAddonList() {
        addonListModel.clear()
        
        // Get all addons
        let addons = addonRepo.getAllAddons()
        
        for (let i = 0; i < addons.length; i++) {
            let addon = addons[i]
            addonListModel.append({
                id: addon.id || "",
                name: addon.name || addon.id || "Unknown",
                version: addon.version || "N/A",
                enabled: addon.enabled || false
            })
        }
        
        addonStatusText.text = "Total addons: " + addonListModel.count + " (enabled: " + addonRepo.getEnabledAddonsCount() + ")"
    }
    
    // Function to refresh catalog list
    function refreshCatalogList() {
        catalogListModel.clear()
        
        // Get all available catalogs
        let catalogs = catalogPrefsService.getAvailableCatalogs()
        
        for (let i = 0; i < catalogs.length; i++) {
            let catalog = catalogs[i]
            catalogListModel.append({
                addonId: catalog.addonId || "",
                addonName: catalog.addonName || "",
                catalogType: catalog.catalogType || "",
                catalogId: catalog.catalogId || "",
                catalogName: catalog.catalogName || "Unknown",
                enabled: catalog.enabled !== undefined ? catalog.enabled : true,
                isHeroSource: catalog.isHeroSource !== undefined ? catalog.isHeroSource : false,
                uniqueId: catalog.uniqueId || ""
            })
        }
        
        catalogStatusText.text = "Total catalogs: " + catalogListModel.count
        catalogStatusText.color = "#4CAF50"
    }
    
    // File export service
    property FileExportService fileExportService: FileExportService {
        id: fileExportService
        
        onFileWritten: function(filePath) {
            catalogStatusText.text = "\u2713 File saved to: " + filePath
            catalogStatusText.color = "#4CAF50"
        }
        
        onError: function(message) {
            catalogStatusText.text = "\u2717 Error saving file: " + message
            catalogStatusText.color = "#F44336"
        }
    }
    
    // Library service for exporting catalog data (singleton, accessed directly)
    property LibraryService libraryService: LibraryService
    
    Connections {
        target: libraryService
        function onCatalogsLoaded(sections) {
            exportTimeout.stop()
            exportCatalogDataToFile(sections, false) // false = processed data
        }
        
        function onRawCatalogsLoaded(rawData) {
            exportTimeout.stop()
            exportCatalogDataToFile(rawData, true) // true = raw data
        }
        
        function onError(message) {
            catalogStatusText.text = "\u2717 Export error: " + message
            catalogStatusText.color = "#F44336"
        }
    }
    
    property Timer exportTimeout: Timer {
        id: exportTimeout
        interval: 30000 // 30 seconds timeout
        onTriggered: {
            catalogStatusText.text = "\u2717 Export timeout: Catalogs did not load within 30 seconds."
            catalogStatusText.color = "#F44336"
        }
    }
    
    function exportCatalogData() {
        console.log("[Settings] ===== exportCatalogData() called =====")
        console.log("[Settings] libraryService:", libraryService)
        
        if (!libraryService) {
            console.error("[Settings] ERROR: libraryService is null!")
            catalogStatusText.text = "\u2717 Error: LibraryService not available"
            catalogStatusText.color = "#F44336"
            return
        }
        
        catalogStatusText.text = "Loading catalogs for export... (this may take a moment)"
        catalogStatusText.color = "#2196F3"
        exportTimeout.start()
        console.log("[Settings] Started export timeout timer (30s)")
        console.log("[Settings] Calling libraryService.loadCatalogs()")
        libraryService.loadCatalogs()
    }
    
    function exportCatalogDataToFile(sections, isRaw) {
        if (!sections || sections.length === 0) {
            catalogStatusText.text = "\u2717 No catalog data to export. Make sure addons are enabled and catalogs are loaded."
            catalogStatusText.color = "#FF9800"
            return
        }
        
        // Convert to JSON
        let jsonData = {
            timestamp: new Date().toISOString(),
            dataType: isRaw ? "raw" : "processed",
            sectionsCount: sections.length,
            sections: []
        }

        for (let i = 0; i < sections.length; i++) {
            let section = sections[i]
            let sectionData = {
                name: section.name || section.catalogName || "",
                type: section.type || section.catalogType || "",
                addonId: section.addonId || "",
                catalogId: section.catalogId || "",
                itemsCount: (section.items || []).length,
                items: []
            }

            let items = section.items || []
            for (let j = 0; j < items.length; j++) {
                let item = items[j]
                
                if (isRaw) {
                    // For raw data, include everything as-is
                    sectionData.items.push(item)
                } else {
                    // For processed data, extract specific fields
                    sectionData.items.push({
                        id: item.id || "",
                        title: item.title || item.name || "",
                        type: item.type || "",
                        poster: item.poster || item.posterUrl || "",
                        background: item.background || item.backdropUrl || "",
                        logo: item.logo || item.logoUrl || "",
                        description: item.description || "",
                        year: item.year || 0,
                        rating: item.rating || "",
                        imdbId: item.imdbId || "",
                        tmdbId: item.tmdbId || "",
                        traktId: item.traktId || ""
                    })
                }
            }

            jsonData.sections.push(sectionData)
        }

        // Convert to JSON string
        let jsonString = JSON.stringify(jsonData, null, 2)
        
        // Generate filename with timestamp
        let timestamp = new Date().toISOString().replace(/[:.]/g, "-").substring(0, 19)
        let dataType = isRaw ? "raw" : "processed"
        let fileName = "catalog_data_" + dataType + "_" + timestamp + ".json"
        
        // Get Downloads directory path
        let downloadsPath = fileExportService.getDownloadsPath()
        let filePath = downloadsPath + "/" + fileName
        
        // Write to file
        let success = fileExportService.writeTextFile(filePath, jsonString)
        
        if (success) {
            let totalItems = jsonData.sections.reduce((sum, s) => sum + s.itemsCount, 0)
            catalogStatusText.text = "\u2713 Exported " + sections.length + " sections with " + totalItems + 
                                     " items to:\n" + filePath
            catalogStatusText.color = "#4CAF50"
        } else {
            catalogStatusText.text = "\u2717 Failed to write file."
            catalogStatusText.color = "#F44336"
        }
    }
    
    // Delete confirmation dialog
    MessageDialog {
        id: deleteConfirmDialog
        property string addonName: ""
        property string addonId: ""
        
        title: "Remove Addon"
        text: "Are you sure you want to remove \"" + addonName + "\"?\n\nThis action cannot be undone."
        buttons: MessageDialog.Ok | MessageDialog.Cancel
        modality: Qt.WindowModal
        
        onAccepted: {
            if (addonId !== "") {
                addonRepo.removeAddon(addonId)
            }
        }
    }
    
    Component.onCompleted: {
        refreshAddonList()
        refreshCatalogList()
    }
}