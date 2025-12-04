#ifndef ADDON_CONFIG_H
#define ADDON_CONFIG_H

#include <QString>
#include <QDateTime>
#include <QJsonArray>
#include <QStringList>
// #include "addon_manifest.h" // Uncomment if strictly needed, otherwise forward declare

struct AddonRecord; // Forward declaration

struct AddonConfig
{
    // 1. Public members with default initialization (Modern C++)
    QString id;
    QString name;
    QString version;
    QString description;
    QString manifestUrl;
    QString baseUrl;
    bool enabled = true; // Default to true
    QString manifestData;
    QJsonArray resources;
    QStringList types;
    QDateTime createdAt;
    QDateTime updatedAt;

    // 2. Static factory method
    static AddonConfig fromDatabase(const AddonRecord& record);

    // 3. Conversion method
    AddonRecord toDatabaseRecord() const;

    // Optional: Helper to check validity
    bool isValid() const { return !id.isEmpty(); }
};

#endif // ADDON_CONFIG_H