#ifndef NAVIGATION_SERVICE_H
#define NAVIGATION_SERVICE_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QtQmlIntegration/qqmlintegration.h>

/**
 * @brief Centralized navigation service for managing app navigation and data passing
 * 
 * Replaces the "pending properties" pattern with a clean, signal-based navigation API.
 * All navigation requests go through this service, ensuring consistent behavior.
 */
class NavigationService : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit NavigationService(QObject* parent = nullptr);

    /**
     * @brief Navigate to detail screen for a content item
     * @param contentId The content identifier (IMDB ID, TMDB ID, etc.)
     * @param type Content type ("movie", "tv", "series")
     * @param addonId Optional addon ID if content is from a specific addon
     */
    Q_INVOKABLE void navigateToDetail(const QString& contentId, const QString& type, const QString& addonId = "");

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
     * @brief Navigate to a specific screen by index
     * @param screenIndex Screen index (0=Home, 1=Library, 2=Settings, 3=Search, 4=Detail, 5=Player)
     */
    Q_INVOKABLE void navigateToScreen(int screenIndex);

signals:
    /**
     * @brief Emitted when detail screen navigation is requested
     */
    void detailRequested(const QString& contentId, const QString& type, const QString& addonId);

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

private:
    // Navigation history for back button support
    QList<int> m_navigationHistory;
    static constexpr int MAX_HISTORY = 50;
};

#endif // NAVIGATION_SERVICE_H

