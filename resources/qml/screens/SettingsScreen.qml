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
                                        color: parent.pressed ? "#985eff" : "#bb86fc"
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
                        
                        Text {
                            text: "Trakt Authentication"
                            font.pixelSize: 24
                            font.bold: true
                            color: "#ffffff"
                        }
                        
                        TraktAuthService {
                            id: traktAuth
                            
                            onDeviceCodeGenerated: function(userCode, verificationUrl, expiresIn) {
                                deviceCodeText.text = "Go to: " + verificationUrl + "\nEnter code: " + userCode
                                deviceCodeText.visible = true
                                loginButton.enabled = false
                                pollingTimer.start()
                            }
                            
                            onAuthenticationStatusChanged: function(authenticated) {
                                if (authenticated) {
                                    deviceCodeText.visible = false
                                    pollingTimer.stop()
                                    traktAuth.getCurrentUser()
                                    loginButton.text = "Logout"
                                } else {
                                    loginButton.text = "Login with Trakt"
                                    currentUserText.text = ""
                                }
                            }
                            
                            onUserInfoFetched: function(username, slug) {
                                currentUserText.text = "Logged in as: " + username
                                currentUserText.color = "#4CAF50"
                            }
                            
                            onError: function(message) {
                                traktStatusText.text = "✗ Error: " + message
                                traktStatusText.color = "#F44336"
                                deviceCodeText.visible = false
                                pollingTimer.stop()
                                loginButton.enabled = true
                            }
                            
                            Component.onCompleted: {
                                checkAuthentication()
                            }
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
                            text: traktAuth.isAuthenticated ? "Logout" : "Login with Trakt"
                            enabled: traktAuth.isConfigured
                            
                            background: Rectangle {
                                color: parent.pressed ? "#985eff" : "#bb86fc"
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
                                if (traktAuth.isAuthenticated) {
                                    traktAuth.logout()
                                    currentUserText.text = ""
                                } else {
                                    traktAuth.generateDeviceCode()
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
                        
                        // Polling timer for device code
                        Timer {
                            id: pollingTimer
                            interval: 5000  // Poll every 5 seconds
                            repeat: true
                            running: false
                            
                            onTriggered: {
                                if (traktAuth.isAuthenticated) {
                                    stop()
                                } else {
                                    // The service handles polling internally, but we can check status
                                    traktAuth.checkAuthentication()
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
                                    color: parent.pressed ? "#985eff" : "#bb86fc"
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
                                                    ? (parent.pressed ? "#985eff" : "#bb86fc")
                                                    : (parent.pressed ? "#3d3d3d" : "#2d2d2d")
                                                radius: 4
                                                border.width: 1
                                                border.color: model.isHeroSource ? "#bb86fc" : "#3d3d3d"
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
    
    Component.onCompleted: {
        refreshAddonList()
        refreshCatalogList()
    }
}

