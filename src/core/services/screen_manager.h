#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

/**
 * @brief Manages screen stack and navigation history
 * 
 * Provides screen identifiers and handles screen transitions.
 * Replaces direct stackLayout.currentIndex manipulation with a clean API.
 */
class ScreenManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int currentScreen READ currentScreen NOTIFY currentScreenChanged)

public:
    explicit ScreenManager(QObject* parent = nullptr);

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
     * @brief Navigate to a specific screen
     * @param screen Screen enum value
     */
    Q_INVOKABLE void navigateTo(Screen screen);

    /**
     * @brief Navigate to a specific screen by index
     * @param screenIndex Screen index (0-5)
     */
    Q_INVOKABLE void navigateToIndex(int screenIndex);

    /**
     * @brief Navigate back to previous screen
     */
    Q_INVOKABLE void navigateBack();

    /**
     * @brief Check if back navigation is available
     */
    Q_INVOKABLE bool canGoBack() const;

signals:
    /**
     * @brief Emitted when current screen changes
     */
    void currentScreenChanged(int screen);

    /**
     * @brief Emitted when navigation to a screen is requested
     */
    void screenChangeRequested(int screenIndex);

private:
    int m_currentScreen = 0;
    QList<int> m_navigationHistory;
    static constexpr int MAX_HISTORY = 50;

    void pushToHistory(int screen);
    int popFromHistory();
};

#endif // SCREEN_MANAGER_H

