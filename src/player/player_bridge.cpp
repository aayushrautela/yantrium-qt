#include "player_bridge.h"
#include <QQuickWindow>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QDebug>
#include <iostream>
#include <mdk/global.h>

// Custom renderer class for MDK video playback
class PlayerRenderer : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions
{
public:
    PlayerRenderer(MDKPlayer *player) : m_player(player), m_fbo(nullptr) {}

    void render() override {
        static int renderCount = 0;
        renderCount++;

        // Make sure we have the right OpenGL context
        initializeOpenGLFunctions();

        if (!m_fbo) {
            return; // No FBO, can't render
        }

        // Render the video using MDK
        if (m_player && m_fbo) {
            // Get the FBO handle (OpenGL framebuffer object name)
            GLuint fboHandle = m_fbo->handle();
            
            // Check state before rendering
            auto state = m_player->currentState();
            
            // Update FBO ID every frame to ensure it's set correctly
            // FBO handle should be > 0 for a created FBO (0 is default framebuffer)
            if (fboHandle > 0) {
                m_player->updateRenderAPIWithFBO(fboHandle);
            } else if (renderCount <= 5) {
                std::cout << "[PlayerRenderer] WARNING: Invalid FBO handle: " << fboHandle << std::endl;
                std::cout.flush();
            }
            
            if (renderCount <= 10 || renderCount % 60 == 0) {
                std::cout << "[PlayerRenderer] frame:" << renderCount 
                          << " state:" << static_cast<int>(state) 
                          << " FBO handle:" << fboHandle
                          << " FBO texture:" << m_fbo->texture() << std::endl;
                std::cout.flush();
            }
            
            // MDK will automatically bind and render to the FBO we specified
            // No need to bind/unbind manually - MDK handles it when fbo >= 0
            m_player->renderVideo();
        }
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override {
        std::cout << "[PlayerRenderer] createFramebufferObject called with size: " << size.width() << "x" << size.height() << std::endl;
        std::cout.flush();

        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        format.setSamples(0); // No multisampling for video

        m_fbo = new QOpenGLFramebufferObject(size, format);
        std::cout << "[PlayerRenderer] FBO created: " << (m_fbo ? "SUCCESS" : "FAILED") << std::endl;
        if (m_fbo) {
            std::cout << "[PlayerRenderer] FBO size: " << m_fbo->size().width() << "x" << m_fbo->size().height() << std::endl;
            std::cout << "[PlayerRenderer] FBO texture: " << m_fbo->texture() << std::endl;
        }
        std::cout.flush();

        return m_fbo;
    }

private:
    MDKPlayer *m_player;
    QOpenGLFramebufferObject *m_fbo;
};

PlayerBridge::PlayerBridge(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
    , m_player(std::make_unique<MDKPlayer>(this))
    , m_source()
    , m_isPlaying(false)
{
    std::cout << "[PlayerBridge] Constructor called - USING QQuickFramebufferObject" << std::endl;
    std::cout.flush();
    qDebug() << "[PlayerBridge] Constructor called - USING QQuickFramebufferObject";

    // Listen for window changes to handle geometry updates
    connect(this, &QQuickItem::windowChanged, this, &PlayerBridge::handleWindowChanged);
    
    // Setup timer to continuously update the FBO while playing
    auto updateTimer = std::make_unique<QTimer>(this);
    connect(updateTimer.get(), &QTimer::timeout, this, [this]() {
        if (m_isPlaying) {
            update(); // Trigger FBO update
        }
    });
    updateTimer->start(33); // ~30 FPS
    m_updateTimer = std::move(updateTimer); // Store as member
    qDebug() << "[PlayerBridge] Update timer started for continuous rendering";
    
    // Connect to MDKPlayer state changes
    connect(m_player.get(), &MDKPlayer::stateChanged, this, [this](int state) {
        // State enum: 0=Stopped/NotRunning, 1=Playing/Running, 2=Paused
        qDebug() << "[PlayerBridge] State changed to:" << state;
        bool playing = (state == 1); // mdk::State::Playing
        if (m_isPlaying != playing) {
            m_isPlaying = playing;
            qDebug() << "[PlayerBridge] isPlaying changed to:" << playing;
            emit isPlayingChanged();
            
            // Keep update loop alive while playing
            if (playing) {
                // Mark the item as needing continuous updates
                update();
                // Also trigger window update
                if (window()) {
                    window()->update();
                }
            }
        }
    });
    
    // Connect to MDKPlayer position changes
    connect(m_player.get(), &MDKPlayer::positionChanged, this, &PlayerBridge::positionChanged);
    qDebug() << "[PlayerBridge] Connected to MDKPlayer signals";
}

void PlayerBridge::handleWindowChanged(QQuickWindow *win)
{
    if (win) {
        qDebug() << "[PlayerBridge] Window changed";

        // Set initial size
        if (width() > 0 && height() > 0) {
            qreal dpr = win->devicePixelRatio();
            int w = static_cast<int>(width() * dpr);
            int h = static_cast<int>(height() * dpr);
            qDebug() << "[PlayerBridge] Setting initial video surface size:" << w << "x" << h;
            m_player->setVideoSurfaceSize(w, h);
        }
    }
}

PlayerBridge::~PlayerBridge()
{
    // MDKPlayer will be deleted as child of this QQuickItem
}

void PlayerBridge::setSource(const QString &source)
{
    std::cout << "[PlayerBridge] setSource called with: " << source.toStdString() << std::endl;
    std::cout.flush();
    qDebug() << "[PlayerBridge] setSource called with:" << source;
    if (m_source != source) {
        m_source = source;
        qDebug() << "[PlayerBridge] Setting media to MDKPlayer";
        m_player->setMedia(source);
        
        // Set video surface size if we have geometry (with HiDPI scaling)
        qDebug() << "[PlayerBridge] Geometry - width:" << width() << "height:" << height();
        if (width() > 0 && height() > 0 && window()) {
            qDebug() << "[PlayerBridge] Window devicePixelRatio:" << window()->devicePixelRatio();
            int w = static_cast<int>(width() * window()->devicePixelRatio());
            int h = static_cast<int>(height() * window()->devicePixelRatio());
            qDebug() << "[PlayerBridge] Setting video surface size to:" << w << "x" << h;
            m_player->setVideoSurfaceSize(w, h);
        } else {
            qDebug() << "[PlayerBridge] WARNING: Cannot set video surface size - width:" << width() 
                     << "height:" << height() << "window:" << (window() ? "exists" : "null");
        }
        
        emit sourceChanged();
        emit durationChanged();
        qDebug() << "[PlayerBridge] Source changed signal emitted";

        // Don't auto-play for now to avoid issues during QML loading
        qDebug() << "[PlayerBridge] Media loaded, ready for manual play";
    } else {
        qDebug() << "[PlayerBridge] Source unchanged, skipping";
    }
}

bool PlayerBridge::isPlaying() const
{
    return m_isPlaying;
}

int64_t PlayerBridge::duration() const
{
    return m_player->duration();
}

int64_t PlayerBridge::position() const
{
    return m_player->position();
}

void PlayerBridge::play()
{
    qDebug() << "[PlayerBridge] play() called";
    m_player->play();
}

void PlayerBridge::pause()
{
    qDebug() << "[PlayerBridge] pause() called";
    m_player->pause();
}

void PlayerBridge::stop()
{
    qDebug() << "[PlayerBridge] stop() called";
    m_player->stop();
}

void PlayerBridge::seek(int64_t ms)
{
    m_player->seek(ms);
}

QQuickFramebufferObject::Renderer *PlayerBridge::createRenderer() const
{
    std::cout << "[PlayerBridge] createRenderer() called - creating PlayerRenderer" << std::endl;
    std::cout.flush();
    return new PlayerRenderer(m_player.get());
}

void PlayerBridge::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    
    if (newGeometry.isValid() && window()) {
        qreal dpr = window()->devicePixelRatio();
        int w = static_cast<int>(newGeometry.width() * dpr);
        int h = static_cast<int>(newGeometry.height() * dpr);
        if (w > 0 && h > 0) {
            std::cout << "[PlayerBridge] geometryChange - setting surface size:" << w << "x" << h << std::endl;
            std::cout.flush();
            m_player->setVideoSurfaceSize(w, h);
        }
    }
}
