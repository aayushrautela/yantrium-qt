#ifndef NAVIGATION_SERVICE_H
#define NAVIGATION_SERVICE_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QList>
#include <QtQmlIntegration/qqmlintegration.h>

/**
 * @brief Unified navigation service for managing app navigation, screen stack, and data passing
 * 
 * Combines navigation requests with screen management. Replaces the "pending properties" pattern
 * with a clean, signal-based navigation API. All navigation requests go through this service,
 * ensuring consistent behavior.
 */
class NavigationService : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int currentScreen READ currentScreen NOTIFY currentScreenChanged)

public:
    explicit NavigationService(QObject* parent = nullptr);

    /**
     * @brief Screen identifiers
     */
    enum Screen {
        Home = 0,
        Library = 1,
        Settings = 2,
        Search = 3,
        Detail = 4,
        Player = 5
    };
    Q_ENUM(Screen)

    /**
     * @brief Get current screen index
     */
    [[nodiscard]] int currentScreen() const { return m_currentScreen; }

    /**
     * @brief Navigate to detail screen for a content item
     * @param contentId The content identifier (IMDB ID, TMDB ID, etc.)
     * @param type Content type ("movie", "tv", "series")
     * @param addonId Optional addon ID if content is from a specific addon
     */
    Q_INVOKABLE void navigateToDetail(const QString& contentId, const QString& type, const QString& addonId = "");
    
    /**
     * @brief Navigate to detail screen for a content item with episode information
     * @param contentId The content identifier (IMDB ID, TMDB ID, etc.) - for episodes, this is the show's ID
     * @param type Content type ("movie", "tv", "series")
     * @param addonId Optional addon ID if content is from a specific addon
     * @param season Season number (for episodes)
     * @param episode Episode number (for episodes)
     */
    Q_INVOKABLE void navigateToDetail(const QString& contentId, const QString& type, const QString& addonId, int season, int episode);

    /**
     * @brief Navigate to video player with stream URL and content data
     * @param streamUrl The stream URL to play
     * @param contentData Content metadata (type, title, description, etc.)
     */
    Q_INVOKABLE void navigateToPlayer(const QString& streamUrl, const QVariantMap& contentData = QVariantMap());

    /**
     * @brief Navigate to search screen with optional query
     * @param query Optional search query to execute immediately
     */
    Q_INVOKABLE void navigateToSearch(const QString& query = "");

    /**
     * @brief Navigate back to previous screen
     */
    Q_INVOKABLE void navigateBack();

    /**
     * @brief Navigate to a specific screen
     * @param screen Screen enum value
     */
    Q_INVOKABLE void navigateTo(Screen screen);

    /**
     * @brief Navigate to a specific screen by index
     * @param screenIndex Screen index (0=Home, 1=Library, 2=Settings, 3=Search, 4=Detail, 5=Player)
     */
    Q_INVOKABLE void navigateToIndex(int screenIndex);

    /**
     * @brief Navigate to a specific screen by index (alias for navigateToIndex)
     * @param screenIndex Screen index (0=Home, 1=Library, 2=Settings, 3=Search, 4=Detail, 5=Player)
     */
    Q_INVOKABLE void navigateToScreen(int screenIndex);

    /**
     * @brief Check if back navigation is available
     */
    Q_INVOKABLE bool canGoBack() const;

signals:
    /**
     * @brief Emitted when detail screen navigation is requested
     * @param season Season number (0 or -1 if not applicable)
     * @param episode Episode number (0 or -1 if not applicable)
     */
    void detailRequested(const QString& contentId, const QString& type, const QString& addonId, int season, int episode);

    /**
     * @brief Emitted when video player navigation is requested
     */
    void playerRequested(const QString& streamUrl, const QVariantMap& contentData);

    /**
     * @brief Emitted when search screen navigation is requested
     */
    void searchRequested(const QString& query);

    /**
     * @brief Emitted when back navigation is requested
     */
    void backRequested();

    /**
     * @brief Emitted when screen navigation is requested
     */
    void screenRequested(int screenIndex);

    /**
     * @brief Emitted when current screen changes
     */
    void currentScreenChanged(int screen);

    /**
     * @brief Emitted when navigation to a screen is requested
     */
    void screenChangeRequested(int screenIndex);

private:
    void pushToHistory(int screen);
    int popFromHistory();

    int m_currentScreen = 0;
    QList<int> m_navigationHistory;
    static constexpr int MAX_HISTORY = 50;
};

#endif // NAVIGATION_SERVICE_H

