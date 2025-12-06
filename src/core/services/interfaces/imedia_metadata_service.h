#ifndef IMEDIA_METADATA_SERVICE_H
#define IMEDIA_METADATA_SERVICE_H

#include <QString>
#include <QVariantMap>

/**
 * @brief Interface for media metadata service operations
 * 
 * Pure abstract interface - no QObject inheritance to avoid diamond inheritance.
 * Signals are defined in concrete implementations.
 */
class IMediaMetadataService
{
public:
    virtual ~IMediaMetadataService() = default;
    
    virtual void getCompleteMetadata(const QString& contentId, const QString& type) = 0;
    virtual void getCompleteMetadataFromTmdbId(int tmdbId, const QString& type) = 0;
    
    // Cache management
    virtual void clearMetadataCache() = 0;
    [[nodiscard]] virtual int getMetadataCacheSize() const = 0;
};

#endif // IMEDIA_METADATA_SERVICE_H

