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
                        
                        // Installed addons list
                        Column {
                            width: parent.width
                            spacing: 10
                            Text { text: "INSTALLED ADDONS"; font.pixelSize: 14; font.bold: true; font.letterSpacing: 0.5; color: "#aaaaaa"; visible: addonListModel.count > 0 }
                            
                            ListView {
                                width: parent.width
                                height: Math.min(addonListModel.count * 90, 500)
                                model: addonListModel
                                clip: true; spacing: 12; visible: addonListModel.count > 0
                                
                                delegate: Rectangle {
                                    width: ListView.view.width; height: 85; color: "#1a1a1a"; radius: 8; border.width: 1; border.color: "#2d2d2d"
                                    
                                    Row {
                                        anchors.fill: parent; anchors.margins: 16; spacing: 16
                                        Column {
                                            width: parent.width - toggleSwitch.width - deleteButton.width - 40; anchors.verticalCenter: parent.verticalCenter; spacing: 8
                                            Text { width: parent.width; text: model.name || model.id || "Unknown"; font.pixelSize: 16; font.bold: true; color: "#ffffff"; elide: Text.ElideRight }
                                            Row {
                                                spacing: 12
                                                Text { text: "v" + (model.version || "N/A"); font.pixelSize: 12; color: "#aaaaaa" }
                                                Row {
                                                    spacing: 6; anchors.verticalCenter: parent.verticalCenter
                                                    Rectangle { width: 8; height: 8; radius: 4; anchors.verticalCenter: parent.verticalCenter; color: (model.enabled || false) ? "#4CAF50" : "#666666" }
                                                    Text { text: (model.enabled || false) ? "Active" : "Inactive"; font.pixelSize: 12; color: (model.enabled || false) ? "#4CAF50" : "#666666"; anchors.verticalCenter: parent.verticalCenter }
                                                }
                                            }
                                        }
                                        Switch {
                                            id: toggleSwitch; anchors.verticalCenter: parent.verticalCenter; checked: model.enabled || false
                                            onToggled: { if (checked) addonRepo.enableAddon(model.id); else addonRepo.disableAddon(model.id); refreshAddonList() }
                                        }
                                        
                                        // --- DELETE BUTTON ---
                                        Button {
                                            id: deleteButton
                                            width: 40
                                            height: 40
                                            anchors.verticalCenter: parent.verticalCenter

                                            // 1. Transparent Background
                                            background: Rectangle {
                                                color: parent.pressed ? "#22ffffff" : "transparent"
                                                radius: 4
                                            }

                                            contentItem: Item {
                                                Image {
                                                    id: deleteIcon
                                                    source: "qrc:/assets/icons/delete.svg"
                                                    
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
                                            onClicked: { deleteConfirmDialog.addonName = model.name || model.id || "Unknown"; deleteConfirmDialog.addonId = model.id; deleteConfirmDialog.open() }
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
                // TRAKT SECTION
                // ==========================================
                Rectangle {
                    width: parent.width
                    height: traktSection.height + 40
                    color: "#1a1a1a"
                    radius: 8
                    
                    Column {
                        id: traktSection
                        width: parent.width - 40; anchors.left: parent.left; anchors.top: parent.top; anchors.margins: 20; spacing: 20
                        property int pendingSyncs: 0
                        function startResync() { pendingSyncs = 2; traktStatusText.text = "Resyncing..."; traktStatusText.color = "#2196F3"; resyncButton.enabled = false; TraktCoreService.resyncWatchedHistory() }
                        
                        Row {
                            width: parent.width; spacing: 10
                            Column {
                                width: parent.width - (loginButton.width + (resyncButton.visible ? resyncButton.width + parent.spacing : 0) + parent.spacing); spacing: 4
                                Text { text: "Trakt Authentication"; font.pixelSize: 24; font.bold: true; color: "#ffffff" }
                                Text { text: "Sync your watch history."; font.pixelSize: 14; color: "#aaaaaa"; wrapMode: Text.WordWrap; width: parent.width }
                            }
                            Button {
                                id: loginButton; width: 200; height: 44
                                text: TraktAuthService.isAuthenticated ? "Logout" : "Login with Trakt"
                                enabled: TraktAuthService.isConfigured
                                background: Rectangle { color: parent.pressed ? "#e0e0e0" : "#ffffff"; radius: 4 }
                                contentItem: Text { text: parent.text; color: "#000000"; font.bold: true; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                onClicked: { if (TraktAuthService.isAuthenticated) { TraktAuthService.logout(); currentUserText.text = "" } else { TraktAuthService.generateDeviceCode() } }
                            }
                            
                            // --- RESYNC BUTTON ---
                            Button {
                                id: resyncButton
                                width: 44
                                height: 44
                                visible: TraktAuthService.isAuthenticated
                                enabled: TraktAuthService.isAuthenticated

                                // 1. Transparent Background
                                background: Rectangle {
                                    color: parent.pressed ? "#22ffffff" : "transparent"
                                    radius: 4
                                }

                                contentItem: Item {
                                    Image {
                                        id: resyncIcon
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
                                onClicked: { traktSection.startResync() }
                            }
                        }
                        
                        Connections { target: TraktAuthService; function onDeviceCodeGenerated(u,v,e){ deviceCodeText.text=v+" : "+u; deviceCodeText.visible=true; pollingTimer.start() } function onAuthenticationStatusChanged(a){ if(a){deviceCodeText.visible=false; pollingTimer.stop(); TraktAuthService.getCurrentUser(); loginButton.text="Logout"}else{loginButton.text="Login"; currentUserText.text=""} } function onUserInfoFetched(u,s){ currentUserText.text="Logged in: "+u; currentUserText.color="#4CAF50" } function onError(m){traktStatusText.text="Error: "+m; traktStatusText.color="#F44336"} }
                        Component.onCompleted: { TraktAuthService.checkAuthentication() }
                        Text { id: currentUserText; width: parent.width; font.pixelSize: 14; color: "#aaaaaa"; wrapMode: Text.WordWrap }
                        Text { id: deviceCodeText; width: parent.width; font.pixelSize: 14; color: "#ffffff"; wrapMode: Text.WordWrap; visible: false }
                        Text { id: traktStatusText; width: parent.width; font.pixelSize: 14; color: "#aaaaaa"; wrapMode: Text.WordWrap }
                        Connections { target: TraktCoreService; function onWatchedMoviesSynced(a,u){ traktStatusText.text="Movies synced: "+a; traktSection.pendingSyncs=Math.max(0,traktSection.pendingSyncs-1); if(traktSection.pendingSyncs===0) resyncButton.enabled=true } function onWatchedShowsSynced(a,u){ traktStatusText.text="Shows synced: "+a; traktSection.pendingSyncs=Math.max(0,traktSection.pendingSyncs-1); if(traktSection.pendingSyncs===0) resyncButton.enabled=true } function onSyncError(t,m){ traktStatusText.text="Error: "+m; traktSection.pendingSyncs=Math.max(0,traktSection.pendingSyncs-1); if(traktSection.pendingSyncs===0) resyncButton.enabled=true } }
                        Timer { id: pollingTimer; interval: 5000; repeat: true; running: false; onTriggered: { if(TraktAuthService.isAuthenticated) stop(); else TraktAuthService.checkAuthentication() } }
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
    function refreshAddonList() { addonListModel.clear(); let addons = addonRepo.getAllAddons(); for(let i=0; i<addons.length; i++) addonListModel.append({id: addons[i].id, name: addons[i].name, version: addons[i].version, enabled: addons[i].enabled}); addonStatusText.text="Total: "+addonListModel.count }
    function refreshCatalogList() { catalogListModel.clear(); let catalogs = catalogPrefsService.getAvailableCatalogs(); for(let i=0; i<catalogs.length; i++) catalogListModel.append({addonId: catalogs[i].addonId, addonName: catalogs[i].addonName, catalogType: catalogs[i].catalogType, catalogId: catalogs[i].catalogId, catalogName: catalogs[i].catalogName, enabled: catalogs[i].enabled, isHeroSource: catalogs[i].isHeroSource}); catalogStatusText.text="Total: "+catalogListModel.count }
    
    // Delete Dialog
    MessageDialog { id: deleteConfirmDialog; property string addonName; property string addonId; title: "Remove Addon"; text: "Remove \""+addonName+"\"?"; buttons: MessageDialog.Ok|MessageDialog.Cancel; onAccepted: if(addonId) addonRepo.removeAddon(addonId) }
    Component.onCompleted: { refreshAddonList(); refreshCatalogList() }
}