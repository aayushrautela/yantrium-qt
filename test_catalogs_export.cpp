#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include "core/database/database_manager.h"
#include "core/services/library_service.h"
#include <iostream>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    std::cout << "=== Catalog Data Export Test ===" << std::endl;
    
    // Initialize database
    DatabaseManager& dbManager = DatabaseManager::instance();
    if (!dbManager.initialize()) {
        std::cerr << "ERROR: Failed to initialize database" << std::endl;
        return -1;
    }
    std::cout << "Database initialized" << std::endl;
    
    // Create library service
    LibraryService* libraryService = new LibraryService();
    
    // Connect to signals
    QObject::connect(libraryService, &LibraryService::catalogsLoaded, [](const QVariantList& sections) {
        std::cout << "Catalogs loaded: " << sections.size() << " sections" << std::endl;
        
        // Convert to JSON
        QJsonObject root;
        root["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        root["sectionsCount"] = sections.size();
        
        QJsonArray sectionsArray;
        
        for (const QVariant& sectionVar : sections) {
            QVariantMap section = sectionVar.toMap();
            QJsonObject sectionObj;
            sectionObj["name"] = section["name"].toString();
            sectionObj["type"] = section["type"].toString();
            sectionObj["addonId"] = section["addonId"].toString();
            
            QVariantList items = section["items"].toList();
            sectionObj["itemsCount"] = items.size();
            
            QJsonArray itemsArray;
            for (const QVariant& itemVar : items) {
                QVariantMap item = itemVar.toMap();
                QJsonObject itemObj;
                itemObj["id"] = item["id"].toString();
                itemObj["title"] = item["title"].toString();
                itemObj["name"] = item["name"].toString();
                itemObj["type"] = item["type"].toString();
                itemObj["poster"] = item["poster"].toString();
                itemObj["posterUrl"] = item["posterUrl"].toString();
                itemObj["background"] = item["background"].toString();
                itemObj["backdropUrl"] = item["backdropUrl"].toString();
                itemObj["logo"] = item["logo"].toString();
                itemObj["logoUrl"] = item["logoUrl"].toString();
                itemObj["description"] = item["description"].toString();
                itemObj["year"] = item["year"].toInt();
                itemObj["rating"] = item["rating"].toString();
                itemObj["imdbId"] = item["imdbId"].toString();
                itemObj["tmdbId"] = item["tmdbId"].toString();
                itemObj["traktId"] = item["traktId"].toString();
                
                itemsArray.append(itemObj);
            }
            
            sectionObj["items"] = itemsArray;
            sectionsArray.append(sectionObj);
            
            std::cout << "  Section: " << section["name"].toString().toStdString() 
                      << " (" << section["type"].toString().toStdString() << ") "
                      << "from addon: " << section["addonId"].toString().toStdString()
                      << " - " << items.size() << " items" << std::endl;
        }
        
        root["sections"] = sectionsArray;
        
        // Write to file
        QString fileName = QString("catalog_data_%1.json")
            .arg(QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd_hh-mm-ss"));
        
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QJsonDocument doc(root);
            file.write(doc.toJson(QJsonDocument::Indented));
            file.close();
            std::cout << "\n✓ Data exported to: " << fileName.toStdString() << std::endl;
            std::cout << "Total items: " << root["sectionsCount"].toInt() << " sections" << std::endl;
        } else {
            std::cerr << "ERROR: Failed to write file: " << fileName.toStdString() << std::endl;
        }
        
        QCoreApplication::exit(0);
    });
    
    QObject::connect(libraryService, &LibraryService::error, [](const QString& message) {
        std::cerr << "ERROR: " << message.toStdString() << std::endl;
        QCoreApplication::exit(1);
    });
    
    // Load catalogs
    std::cout << "Loading catalogs..." << std::endl;
    libraryService->loadCatalogs();
    
    // Run event loop
    return app.exec();
}




