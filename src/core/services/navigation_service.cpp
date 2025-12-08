#include "navigation_service.h"
#include "logging_service.h"

NavigationService::NavigationService(QObject* parent)
    : QObject(parent)
{
    LoggingService::logInfo("NavigationService", "Initialized");
}

void NavigationService::navigateToDetail(const QString& contentId, const QString& type, const QString& addonId)
{
    if (contentId.isEmpty() || type.isEmpty()) {
        LoggingService::logWarning("NavigationService", "navigateToDetail called with empty contentId or type");
        return;
    }
    
    LoggingService::logInfo("NavigationService", QString("Navigating to detail - contentId: %1, type: %2, addonId: %3").arg(contentId, type, addonId));
    emit detailRequested(contentId, type, addonId);
}

void NavigationService::navigateToPlayer(const QString& streamUrl, const QVariantMap& contentData)
{
    if (streamUrl.isEmpty()) {
        LoggingService::logWarning("NavigationService", "navigateToPlayer called with empty streamUrl");
        return;
    }
    
    LoggingService::logInfo("NavigationService", QString("Navigating to player - streamUrl: %1").arg(streamUrl));
    emit playerRequested(streamUrl, contentData);
}

void NavigationService::navigateToSearch(const QString& query)
{
    LoggingService::logInfo("NavigationService", QString("Navigating to search - query: %1").arg(query));
    emit searchRequested(query);
}

void NavigationService::navigateBack()
{
    LoggingService::logInfo("NavigationService", "Navigating back");
    emit backRequested();
}

void NavigationService::navigateToScreen(int screenIndex)
{
    if (screenIndex < 0 || screenIndex > 5) {
        LoggingService::logWarning("NavigationService", QString("navigateToScreen called with invalid index: %1").arg(screenIndex));
        return;
    }
    
    LoggingService::logInfo("NavigationService", QString("Navigating to screen index: %1").arg(screenIndex));
    emit screenRequested(screenIndex);
}

