#ifndef ISTREAM_SERVICE_H
#define ISTREAM_SERVICE_H

#include <QVariantList>
#include <QVariantMap>
#include <QString>

/**
 * @brief Interface for stream service operations
 * 
 * Pure abstract interface - no QObject inheritance to avoid diamond inheritance.
 * Signals are defined in concrete implementations.
 */
class IStreamService
{
public:
    virtual ~IStreamService() = default;
    
    // Get streams for a catalog item
    // itemData: QVariantMap with id, type, name, etc.
    // episodeId: Optional episode ID for TV shows (format: "S01E01")
    virtual void getStreamsForItem(const QVariantMap& itemData, const QString& episodeId = QString()) = 0;
};

#endif // ISTREAM_SERVICE_H

