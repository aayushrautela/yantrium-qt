import QtQuick
import QtQuick.Controls
import Yantrium.Services 1.0
import Qt.labs.platform
import QtQuick.Dialogs

ApplicationWindow {
    id: root
    width: 800
    height: 600
    visible: true
    title: "Catalog Test - Raw Data Export"

    property LibraryService libraryService: LibraryService {
        id: libraryService
        
        onCatalogsLoaded: function(sections) {
            console.log("[Test] Catalogs loaded, sections:", sections.length)
            exportToFile(sections)
        }
        
        onError: function(message) {
            console.error("[Test] Error:", message)
            statusText.text = "Error: " + message
            statusText.color = "#ff0000"
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        Text {
            text: "Catalog Data Export Test"
            font.pixelSize: 24
            font.bold: true
            color: "#ffffff"
        }

        Row {
            spacing: 10
            
            Button {
                text: "Load Catalogs"
                onClicked: {
                    statusText.text = "Loading catalogs..."
                    statusText.color = "#ffffff"
                    outputText.text = ""
                    libraryService.loadCatalogs()
                }
            }
            
            Button {
                text: "Save to File"
                enabled: outputText.text.length > 0
                onClicked: {
                    if (outputText.text.length > 0) {
                        writeToFile("", outputText.text)
                    }
                }
            }
        }

        Text {
            id: statusText
            text: "Click 'Load Catalogs' to start"
            font.pixelSize: 14
            color: "#aaaaaa"
            wrapMode: Text.WordWrap
            width: parent.width
        }

        ScrollView {
            width: parent.width
            height: parent.height - 200
            clip: true

            TextArea {
                id: outputText
                readOnly: true
                font.family: "monospace"
                font.pixelSize: 12
                color: "#ffffff"
                background: Rectangle {
                    color: "#1a1a1a"
                }
            }
        }
    }

    function exportToFile(sections) {
        console.log("[Test] Exporting", sections.length, "sections to file")
        
        // Convert to JSON
        let jsonData = {
            timestamp: new Date().toISOString(),
            sectionsCount: sections.length,
            sections: []
        }

        for (let i = 0; i < sections.length; i++) {
            let section = sections[i]
            let sectionData = {
                name: section.name || "",
                type: section.type || "",
                addonId: section.addonId || "",
                itemsCount: (section.items || []).length,
                items: []
            }

            let items = section.items || []
            for (let j = 0; j < items.length; j++) {
                let item = items[j]
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

            jsonData.sections.push(sectionData)
        }

        // Convert to JSON string with pretty formatting
        let jsonString = JSON.stringify(jsonData, null, 2)
        
        // Display in text area
        outputText.text = jsonString
        
        // Write to file
        let fileName = "catalog_data_" + new Date().toISOString().replace(/[:.]/g, "-") + ".json"
        let filePath = StandardPaths.writableLocation(StandardPaths.DocumentsLocation) + "/" + fileName
        
        console.log("[Test] Writing to file:", filePath)
        
        // Use FileDialog or write directly
        // For now, we'll use a simple approach with File I/O
        writeToFile(filePath, jsonString)
        
        statusText.text = "✓ Exported " + sections.length + " sections with " + 
                         jsonData.sections.reduce((sum, s) => sum + s.itemsCount, 0) + 
                         " total items to: " + fileName
        statusText.color = "#00ff00"
    }

    FileDialog {
        id: saveDialog
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: ["JSON files (*.json)", "All files (*)"]
        
        onAccepted: {
            let fileUrl = selectedFile
            console.log("[Test] Saving to:", fileUrl)
            
            // Use XMLHttpRequest to write file (works in QML)
            let xhr = new XMLHttpRequest()
            xhr.open("PUT", fileUrl)
            xhr.setRequestHeader("Content-Type", "application/json")
            xhr.send(outputText.text)
            
            xhr.onreadystatechange = function() {
                if (xhr.readyState === XMLHttpRequest.DONE) {
                    if (xhr.status === 0 || xhr.status === 200) {
                        console.log("[Test] File saved successfully")
                        statusText.text = "✓ File saved to: " + fileUrl.toString().replace("file://", "")
                        statusText.color = "#00ff00"
                    } else {
                        console.error("[Test] Failed to save file, status:", xhr.status)
                        statusText.text = "✗ Failed to save file. You can copy the content from the text area below."
                        statusText.color = "#ffaa00"
                    }
                }
            }
        }
    }

    function writeToFile(filePath, content) {
        // Show save dialog
        let fileName = "catalog_data_" + new Date().toISOString().replace(/[:.]/g, "-") + ".json"
        saveDialog.currentFile = fileName
        saveDialog.open()
    }

    Component.onCompleted: {
        console.log("[Test] Test window loaded")
        console.log("[Test] LibraryService:", libraryService)
    }
}

