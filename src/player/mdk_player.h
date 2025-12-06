#ifndef MDK_PLAYER_H
#define MDK_PLAYER_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <memory>
#include <mdk/Player.h>
#include <mdk/RenderAPI.h>

class MDKPlayer : public QObject
{
    Q_OBJECT
    
public:
    explicit MDKPlayer(QObject *parent = nullptr);
    ~MDKPlayer();
    
    // Player control methods
    void setMedia(const QString &url);
    void play();
    void pause();
    void stop();
    void seek(int64_t ms);
    void setVolume(float vol);
    void setVideoSurfaceSize(int width, int height);
    void renderVideo();
    void updateRenderAPI(); // Update RenderAPI with current OpenGL context
    void updateRenderAPIWithFBO(int fboId); // Update RenderAPI with FBO ID for rendering

    // State checking
    bool waitForState(mdk::State state, long timeout = -1);
    mdk::State currentState() const;
    
    // Accessors
    int64_t duration() const;
    int64_t position() const;
    
signals:
    void stateChanged(int state);
    void positionChanged();
    
private:
    mdk::Player m_player;
    mdk::GLRenderAPI m_renderAPI;
    bool m_renderAPISetup;
    int64_t m_lastPosition;
    std::unique_ptr<QTimer> m_positionTimer;
    
    void updatePosition();
};

#endif // MDK_PLAYER_H
