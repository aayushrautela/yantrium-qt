#include "navigation_service.h"
#include "logging_service.h"

NavigationService::NavigationService(QObject* parent)
    : QObject(parent)
    , m_currentScreen(Home)
{
    // Initialize history with home screen
    m_navigationHistory.append(Home);
    LoggingService::logInfo("NavigationService", "Initialized with Home screen");
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
    // Handle screen navigation back first - check if we can go back
    if (!canGoBack()) {
        LoggingService::logDebug("NavigationService", QString("Cannot go back - history size: %1, current screen: %2").arg(m_navigationHistory.size()).arg(m_currentScreen));
        // Don't emit backRequested() if there's nowhere to go back to
        // This prevents infinite loops when QML handlers call navigateBack() in response to backRequested()
        return;
    }

    LoggingService::logDebug("NavigationService", QString("Going back - history before: %1, current: %2").arg(m_navigationHistory.size()).arg(m_currentScreen));

    // Only emit backRequested() if we can actually navigate back
    // This prevents circular calls from QML handlers
    emit backRequested();

    int previousScreen = popFromHistory();
    if (previousScreen == -1) {
        // Fallback to home if history is corrupted
        previousScreen = Home;
    }

    LoggingService::logInfo("NavigationService", QString("Navigating back from %1 to %2 (history size after: %3)").arg(m_currentScreen).arg(previousScreen).arg(m_navigationHistory.size()));
    
    m_currentScreen = previousScreen;
    emit currentScreenChanged(m_currentScreen);
    emit screenChangeRequested(previousScreen);
}

void NavigationService::navigateTo(Screen screen)
{
    navigateToIndex(static_cast<int>(screen));
}

void NavigationService::navigateToIndex(int screenIndex)
{
    if (screenIndex < 0 || screenIndex > 5) {
        LoggingService::logWarning("NavigationService", QString("Invalid screen index: %1").arg(screenIndex));
        return;
    }

    if (screenIndex == m_currentScreen) {
        LoggingService::logDebug("NavigationService", QString("Already on screen: %1").arg(screenIndex));
        return;
    }

    LoggingService::logInfo("NavigationService", QString("Navigating from %1 to %2 (history size before: %3)").arg(m_currentScreen).arg(screenIndex).arg(m_navigationHistory.size()));
    
    // Push current screen to history before changing
    // This ensures we can navigate back to where we came from
    pushToHistory(m_currentScreen);
    
    // Update current screen
    m_currentScreen = screenIndex;
    emit currentScreenChanged(m_currentScreen);
    emit screenChangeRequested(screenIndex);
    emit screenRequested(screenIndex);
    
    LoggingService::logDebug("NavigationService", QString("Navigation complete - current: %1, history size: %2").arg(m_currentScreen).arg(m_navigationHistory.size()));
}

void NavigationService::navigateToScreen(int screenIndex)
{
    navigateToIndex(screenIndex);
}

bool NavigationService::canGoBack() const
{
    // Can go back if history has more than just the current screen
    return m_navigationHistory.size() > 1;
}

void NavigationService::pushToHistory(int screen)
{
    // Always push the current screen to history when navigating away
    // This allows proper back navigation even if we're navigating to a screen we've been to before
    m_navigationHistory.append(screen);
    
    // Limit history size
    if (m_navigationHistory.size() > MAX_HISTORY) {
        m_navigationHistory.removeFirst();
    }
    
    LoggingService::logDebug("NavigationService", QString("Pushed screen %1 to history (size: %2)").arg(screen).arg(m_navigationHistory.size()));
}

int NavigationService::popFromHistory()
{
    if (m_navigationHistory.isEmpty()) {
        return -1;
    }

    // Remove current screen from history
    if (!m_navigationHistory.isEmpty()) {
        m_navigationHistory.removeLast();
    }

    // Return the previous screen (now the last in history)
    if (m_navigationHistory.isEmpty()) {
        return Home; // Default to home if history is empty
    }

    return m_navigationHistory.last();
}

