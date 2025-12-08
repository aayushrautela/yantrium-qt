#include "screen_manager.h"
#include "logging_service.h"

ScreenManager::ScreenManager(QObject* parent)
    : QObject(parent)
    , m_currentScreen(Home)
{
    // Initialize history with home screen
    m_navigationHistory.append(Home);
    LoggingService::logInfo("ScreenManager", "Initialized with Home screen");
}

void ScreenManager::navigateTo(Screen screen)
{
    navigateToIndex(static_cast<int>(screen));
}

void ScreenManager::navigateToIndex(int screenIndex)
{
    if (screenIndex < 0 || screenIndex > 5) {
        LoggingService::logWarning("ScreenManager", QString("Invalid screen index: %1").arg(screenIndex));
        return;
    }

    if (screenIndex == m_currentScreen) {
        LoggingService::logDebug("ScreenManager", QString("Already on screen: %1").arg(screenIndex));
        return;
    }

    LoggingService::logInfo("ScreenManager", QString("Navigating from %1 to %2").arg(m_currentScreen).arg(screenIndex));
    
    // Push current screen to history (unless it's the same)
    pushToHistory(m_currentScreen);
    
    // Update current screen
    m_currentScreen = screenIndex;
    emit currentScreenChanged(m_currentScreen);
    emit screenChangeRequested(screenIndex);
}

void ScreenManager::navigateBack()
{
    if (!canGoBack()) {
        LoggingService::logDebug("ScreenManager", "Cannot go back - history is empty");
        return;
    }

    int previousScreen = popFromHistory();
    if (previousScreen == -1) {
        // Fallback to home if history is corrupted
        previousScreen = Home;
    }

    LoggingService::logInfo("ScreenManager", QString("Navigating back from %1 to %2").arg(m_currentScreen).arg(previousScreen));
    
    m_currentScreen = previousScreen;
    emit currentScreenChanged(m_currentScreen);
    emit screenChangeRequested(previousScreen);
}

bool ScreenManager::canGoBack() const
{
    // Can go back if history has more than just the current screen
    return m_navigationHistory.size() > 1;
}

void ScreenManager::pushToHistory(int screen)
{
    // Don't push if it's the same as the last entry
    if (!m_navigationHistory.isEmpty() && m_navigationHistory.last() == screen) {
        return;
    }

    m_navigationHistory.append(screen);
    
    // Limit history size
    if (m_navigationHistory.size() > MAX_HISTORY) {
        m_navigationHistory.removeFirst();
    }
}

int ScreenManager::popFromHistory()
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

