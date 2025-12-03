import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Yantrium.Components 1.0
import Yantrium.Services 1.0

Item {
    id: root
    anchors.fill: parent
    
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
                
                // Addons Section
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
                        
                        Text {
                            text: "Addons"
                            font.pixelSize: 24
                            font.bold: true
                            color: "#ffffff"
                        }
                        
                        AddonRepository {
                            id: addonRepo
                            
                            onAddonInstalled: function(addon) {
                                addonStatusText.text = "✓ Addon installed: " + addon.name
                                addonStatusText.color = "#4CAF50"
                                refreshAddonList()
                            }
                            
                            onAddonRemoved: function(addonId) {
                                addonStatusText.text = "✓ Removed addon: " + addonId
                                addonStatusText.color = "#FF9800"
                                refreshAddonList()
                            }
                            
                            onError: function(errorMessage) {
                                addonStatusText.text = "✗ Error: " + errorMessage
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
                                
                                TextField {
                                    id: manifestUrlField
                                    width: parent.width - installButton.width - 10
                                    height: 40
                                    placeholderText: "Enter addon manifest URL (e.g., https://...)"

                                    background: Rectangle {
                                        color: "#2d2d2d"
                                        border.width: 1
                                        border.color: "#3d3d3d"
                                        radius: 4
                                    }
                                    
                                    color: "#ffffff"
                                    placeholderTextColor: "#666666"
                                }
                                
                                Button {
                                    id: installButton
                                    width: 120
                                    height: 40
                                    text: "Install"
                                    
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
                                text: "Installed Addons (" + addonListModel.count + ")"
                                font.pixelSize: 16
                                font.bold: true
                                color: "#ffffff"
                            }
                            
                            ListView {
                                width: parent.width
                                height: Math.min(addonListModel.count * 80, 400)
                                model: addonListModel
                                clip: true
                                
                                delegate: Rectangle {
                                    width: ListView.view.width
                                    height: 70
                                    color: "#2d2d2d"
                                    radius: 4
                                    
                                    Row {
                                        anchors.fill: parent
                                        anchors.margins: 10
                                        spacing: 15
                                        
                                        Column {
                                            width: parent.width - toggleSwitch.width - removeButton.width - 30
                                            anchors.verticalCenter: parent.verticalCenter
                                            
                                            Text {
                                                width: parent.width
                                                text: model.name || model.id || "Unknown"
                                                font.pixelSize: 16
                                                font.bold: true
                                                color: "#ffffff"
                                                elide: Text.ElideRight
                                            }
                                            
                                            Text {
                                                width: parent.width
                                                text: "Version: " + (model.version || "N/A")
                                                font.pixelSize: 12
                                                color: "#aaaaaa"
                                                elide: Text.ElideRight
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
                                            id: removeButton
                                            width: 80
                                            height: 36
                                            text: "Remove"
                                            
                                            background: Rectangle {
                                                color: parent.pressed ? "#d32f2f" : "#f44336"
                                                radius: 4
                                            }
                                            
                                            contentItem: Text {
                                                text: parent.text
                                                color: "#ffffff"
                                                font.bold: true
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                            }
                                            
                                            onClicked: {
                                                addonRepo.removeAddon(model.id)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                
                // Trakt Authentication Section
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
                        
                        Text {
                            text: "Trakt Authentication"
                            font.pixelSize: 24
                            font.bold: true
                            color: "#ffffff"
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
                                traktStatusText.text = "✗ Error: " + message
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
                                color: parent.pressed ? "#ff9800" : "#ff9800"
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
                                traktStatusText.text = "✗ Sync error (" + syncType + "): " + message
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
                
                // Catalog Management Section
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
                        
                        Text {
                            text: "Catalog Management"
                            font.pixelSize: 24
                            font.bold: true
                            color: "#ffffff"
                        }
                        
                        CatalogPreferencesService {
                            id: catalogPrefsService
                            
                            onCatalogsUpdated: {
                                refreshCatalogList()
                            }
                            
                            onError: function(message) {
                                catalogStatusText.text = "✗ Error: " + message
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
                                    console.log("[Settings] ===== Export (Processed) button clicked =====")
                                    
                                    // Try to get existing catalog sections first
                                    let existingSections = exportLibraryService.getCatalogSections()
                                    console.log("[Settings] Existing catalog sections:", existingSections ? existingSections.length : 0)
                                    
                                    if (existingSections && existingSections.length > 0) {
                                        console.log("[Settings] Using existing catalog data")
                                        exportCatalogDataToFile(existingSections, false)
                                    } else {
                                        console.log("[Settings] No existing data, loading catalogs...")
                                        catalogStatusText.text = "Loading catalogs... (check console for progress)"
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
                                    console.log("[Settings] ===== Export (Raw) button clicked =====")
                                    catalogStatusText.text = "Loading raw catalog data... (check console for progress)"
                                    catalogStatusText.color = "#2196F3"
                                    exportTimeout.start()
                                    exportLibraryService.loadCatalogsRaw()
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
                                text: "Available Catalogs (" + catalogListModel.count + ")"
                                font.pixelSize: 16
                                font.bold: true
                                color: "#ffffff"
                            }
                            
                            ListView {
                                width: parent.width
                                height: Math.min(catalogListModel.count * 100, 500)
                                model: catalogListModel
                                clip: true
                                
                                delegate: Rectangle {
                                    width: ListView.view.width
                                    height: 90
                                    color: "#2d2d2d"
                                    radius: 4
                                    
                                    Row {
                                        anchors.fill: parent
                                        anchors.margins: 10
                                        spacing: 15
                                        
                                        Column {
                                            width: parent.width - heroToggle.width - enableSwitch.width - 30
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
                                                text: model.addonName + " • " + (model.catalogType || "")
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
                                        
                                        // Hero toggle button
                                        Button {
                                            id: heroToggle
                                            width: 80
                                            height: 36
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: model.isHeroSource ? "Hero ✓" : "Hero"
                                            
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
                                        
                                        // Enable/disable switch
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
                        
                        // Empty state
                        Rectangle {
                            width: parent.width
                            height: 100
                            color: "transparent"
                            visible: catalogListModel.count === 0
                            
                            Column {
                                anchors.centerIn: parent
                                spacing: 8
                                
                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "No catalogs available"
                                    font.pixelSize: 16
                                    color: "#aaaaaa"
                                }
                                
                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "Enable addons with catalog support to see catalogs here."
                                    font.pixelSize: 12
                                    color: "#666666"
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
            console.log("[Settings] File written successfully:", filePath)
            catalogStatusText.text = "✓ File saved to: " + filePath
            catalogStatusText.color = "#4CAF50"
        }
        
        onError: function(message) {
            console.error("[Settings] File export error:", message)
            catalogStatusText.text = "✗ Error saving file: " + message
            catalogStatusText.color = "#F44336"
        }
    }
    
    // Library service for exporting catalog data
    property LibraryService libraryService: LibraryService {
        id: exportLibraryService
        
        Component.onCompleted: {
            console.log("[Settings] Export LibraryService created")
        }
        
        onCatalogsLoaded: function(sections) {
            console.log("[Settings] Export: catalogsLoaded signal received, sections:", sections ? sections.length : 0)
            exportTimeout.stop()
            exportCatalogDataToFile(sections, false) // false = processed data
        }
        
        onRawCatalogsLoaded: function(rawData) {
            console.log("[Settings] Export: rawCatalogsLoaded signal received, sections:", rawData ? rawData.length : 0)
            exportTimeout.stop()
            exportCatalogDataToFile(rawData, true) // true = raw data
        }
        
        onError: function(message) {
            console.error("[Settings] Export error:", message)
            catalogStatusText.text = "✗ Export error: " + message
            catalogStatusText.color = "#F44336"
        }
    }
    
    property Timer exportTimeout: Timer {
        id: exportTimeout
        interval: 30000 // 30 seconds timeout
        onTriggered: {
            console.error("[Settings] Export timeout - catalogs did not load within 30 seconds")
            catalogStatusText.text = "✗ Export timeout: Catalogs did not load. Check console for errors."
            catalogStatusText.color = "#F44336"
        }
    }
    
    function exportCatalogData() {
        console.log("[Settings] ===== exportCatalogData() called =====")
        console.log("[Settings] exportLibraryService:", exportLibraryService)
        
        if (!exportLibraryService) {
            console.error("[Settings] ERROR: exportLibraryService is null!")
            catalogStatusText.text = "✗ Error: LibraryService not available"
            catalogStatusText.color = "#F44336"
            return
        }
        
        catalogStatusText.text = "Loading catalogs for export... (this may take a moment)"
        catalogStatusText.color = "#2196F3"
        exportTimeout.start()
        console.log("[Settings] Started export timeout timer (30s)")
        console.log("[Settings] Calling exportLibraryService.loadCatalogs()")
        exportLibraryService.loadCatalogs()
    }
    
    function exportCatalogDataToFile(sections, isRaw) {
        console.log("[Settings] ===== exportCatalogDataToFile() called =====")
        console.log("[Settings] Exporting", sections ? sections.length : 0, "sections to file (raw:", isRaw, ")")
        
        if (!sections || sections.length === 0) {
            console.warn("[Settings] No sections to export!")
            catalogStatusText.text = "✗ No catalog data to export. Make sure addons are enabled and catalogs are loaded."
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
        
        console.log("[Settings] Writing file to:", filePath)
        console.log("[Settings] JSON length:", jsonString.length, "characters")
        
        // Write to file
        let success = fileExportService.writeTextFile(filePath, jsonString)
        
        if (success) {
            let totalItems = jsonData.sections.reduce((sum, s) => sum + s.itemsCount, 0)
            catalogStatusText.text = "✓ Exported " + sections.length + " sections with " + totalItems + 
                                     " items to:\n" + filePath
            catalogStatusText.color = "#4CAF50"
            
            // Also output to console for backup
            console.log("=== CATALOG DATA EXPORT ===")
            console.log("File saved to:", filePath)
            console.log("Total sections:", sections.length)
            console.log("Total items:", totalItems)
        } else {
            catalogStatusText.text = "✗ Failed to write file. Check console for details."
            catalogStatusText.color = "#F44336"
        }
    }
    
    Component.onCompleted: {
        refreshAddonList()
        refreshCatalogList()
    }
}

