#include "mdk_player.h"
#include <mdk/global.h>
#include <QByteArray>
#include <QTimer>
#include <QDebug>
#include <iostream>
#include <vector>
#include <string>

MDKPlayer::MDKPlayer(QObject *parent)
    : QObject(parent)
    , m_renderAPISetup(false)
    , m_lastPosition(-1)
{
    std::cout << "[MDKPlayer] Constructor called" << std::endl;
    std::cout.flush();
    qDebug() << "[MDKPlayer] Constructor called";
    
    // Set video decoders to hardware acceleration
    std::vector<std::string> videoDecoders = {"CUDA", "VAAPI", "MFT", "D3D11", "FFmpeg"};
    m_player.setDecoders(mdk::MediaType::Video, videoDecoders);
    qDebug() << "[MDKPlayer] Video decoders configured";
    
    // Set audio decoders
    std::vector<std::string> audioDecoders = {"FFmpeg"};
    m_player.setDecoders(mdk::MediaType::Audio, audioDecoders);
    qDebug() << "[MDKPlayer] Audio decoders configured";
    
    // Setup state changed callback
    m_player.onStateChanged([this](mdk::State state) {
        std::cout << "[MDKPlayer] State changed to: " << static_cast<int>(state);
        if (state == mdk::State::Playing) {
            std::cout << " (Playing)";
        } else if (state == mdk::State::Paused) {
            std::cout << " (Paused)";
        } else if (state == mdk::State::Stopped) {
            std::cout << " (Stopped)";
        }
        std::cout << std::endl;
        std::cout.flush();
        qDebug() << "[MDKPlayer] State changed to:" << static_cast<int>(state);
        emit stateChanged(static_cast<int>(state));
    });
    
    // Initialize RenderAPI (OpenGL)
    m_renderAPI = mdk::GLRenderAPI();
    m_renderAPI.opengl = 1; // Use OpenGL
    m_renderAPI.opengles = 0; // Not OpenGL ES
    m_renderAPI.profile = mdk::GLRenderAPI::Profile::Core;
    
    // Force the player to render to the current OpenGL context when we call renderVideo()
    m_player.setRenderAPI(&m_renderAPI);
    m_renderAPISetup = true;
    
    qDebug() << "[MDKPlayer] GLRenderAPI initialized and set (OpenGL, Core profile)";
    
    // Setup timer to emit positionChanged signal periodically
    QTimer *positionTimer = new QTimer(this);
    connect(positionTimer, &QTimer::timeout, this, &MDKPlayer::updatePosition);
    positionTimer->start(100); // Update every 100ms
    qDebug() << "[MDKPlayer] Position timer started";
}

MDKPlayer::~MDKPlayer()
{
    // Prevent crashes on exit
    m_player.setNextMedia(nullptr);
    m_player.set(mdk::State::Stopped);
}

void MDKPlayer::play()
{
    std::cout << "[MDKPlayer] play() called" << std::endl;
    std::cout.flush();
    qDebug() << "[MDKPlayer] play() called";

    auto currentState = m_player.state();
    std::cout << "[MDKPlayer] Current state before setting to Playing: " << static_cast<int>(currentState) << std::endl;
    std::cout.flush();

    // If we're in Stopped state, we might need to wait a bit for prepare() to complete
    if (currentState == mdk::State::Stopped) {
        auto mediaStatus = m_player.mediaStatus();
        std::cout << "[MDKPlayer] Media status: " << static_cast<int>(mediaStatus) << std::endl;
        std::cout.flush();
        
        // Wait a short time for media to be ready (prepare() is async)
        // MediaStatus::Prepared = 1<<8 = 256
        // But we'll just try to play anyway
    }

    m_player.set(mdk::State::Playing);

    // Give it a moment for the state to change (it's async)
    // We can't wait too long here as it would block the UI thread
    // The state change callback will handle the actual state update
    std::cout << "[MDKPlayer] State change requested to Playing (async)" << std::endl;
    std::cout.flush();
    qDebug() << "[MDKPlayer] State set to Playing";
}

void MDKPlayer::pause()
{
    qDebug() << "[MDKPlayer] pause() called";
    m_player.set(mdk::State::Paused);
    qDebug() << "[MDKPlayer] State set to Paused";
}

void MDKPlayer::stop()
{
    qDebug() << "[MDKPlayer] stop() called";
    m_player.set(mdk::State::Stopped);
    qDebug() << "[MDKPlayer] State set to Stopped";
}

void MDKPlayer::setMedia(const QString &url)
{
    std::cout << "[MDKPlayer] setMedia called with URL: " << url.toStdString() << std::endl;
    std::cout.flush();
    qDebug() << "[MDKPlayer] setMedia called with URL:" << url;
    // Convert QString to UTF8 and call setMedia, then prepare
    QByteArray urlUtf8 = url.toUtf8();
    qDebug() << "[MDKPlayer] Setting media to MDK player (UTF8 length:" << urlUtf8.length() << ")";
    m_player.setMedia(urlUtf8.constData());
    qDebug() << "[MDKPlayer] Calling prepare()";
    std::cout << "[MDKPlayer] About to call prepare()" << std::endl;
    std::cout.flush();
    m_player.prepare();
    std::cout << "[MDKPlayer] prepare() called (async)" << std::endl;
    std::cout.flush();
    qDebug() << "[MDKPlayer] prepare() called (async)";
    
    // Wait a short time for prepare to start (it's async)
    // The actual preparation will complete asynchronously
    // We can't wait too long here as it would block the UI thread
    
    // Get media info to verify it loaded
    auto info = m_player.mediaInfo();
    qDebug() << "[MDKPlayer] Media info - duration:" << info.duration << "ms"
             << "hasVideo:" << (info.video.size() > 0) << "hasAudio:" << (info.audio.size() > 0);
    if (info.video.size() > 0) {
        qDebug() << "[MDKPlayer] Video stream - index:" << info.video[0].index
                 << "frames:" << info.video[0].frames << "duration:" << info.video[0].duration << "ms";
    }
    
    m_lastPosition = -1; // Reset position tracking
    
    // Flip video vertically to fix upside-down issue
    // scale(1.0, -1.0) flips vertically (negative y scale)
    m_player.scale(1.0, -1.0);
    qDebug() << "[MDKPlayer] Video flipped vertically to fix orientation";
}

void MDKPlayer::seek(int64_t ms)
{
    m_player.seek(ms);
}

void MDKPlayer::setVolume(float vol)
{
    m_player.setVolume(vol);
}

void MDKPlayer::setVideoSurfaceSize(int width, int height)
{
    qDebug() << "[MDKPlayer] setVideoSurfaceSize called with:" << width << "x" << height;
    m_player.setVideoSurfaceSize(width, height);
}

void MDKPlayer::renderVideo()
{
    // RenderAPI is already set up in constructor
    static int renderCount = 0;
    renderCount++;

    // Log first 5 frames and every 60 frames after that
    if (renderCount <= 5 || renderCount % 60 == 0) {
        std::cout << "[MDKPlayer] renderVideo() called, frame:" << renderCount << std::endl;
        std::cout.flush();
        qDebug() << "[MDKPlayer] renderVideo() called, frame:" << renderCount;
    }

    // Check current state and media status before rendering
    auto currentState = m_player.state();
    auto mediaStatus = m_player.mediaStatus();
    if (renderCount <= 5) {
        std::cout << "[MDKPlayer] Current state before renderVideo(): " << static_cast<int>(currentState) << std::endl;
        std::cout << "[MDKPlayer] Media status: " << static_cast<int>(mediaStatus) << std::endl;
        std::cout.flush();
    }

    double timestamp = m_player.renderVideo();

    if (renderCount <= 5 || renderCount % 60 == 0) {
        if (timestamp >= 0) {
            std::cout << "[MDKPlayer] renderVideo() returned timestamp:" << timestamp << std::endl;
            std::cout.flush();
            qDebug() << "[MDKPlayer] renderVideo() returned timestamp:" << timestamp;
        } else {
            std::cout << "[MDKPlayer] WARNING: renderVideo() returned negative timestamp:" << timestamp << std::endl;
            std::cout.flush();
            qDebug() << "[MDKPlayer] WARNING: renderVideo() returned negative timestamp:" << timestamp;
        }
    }
}

int64_t MDKPlayer::duration() const
{
    return m_player.mediaInfo().duration;
}

int64_t MDKPlayer::position() const
{
    return m_player.position();
}

mdk::State MDKPlayer::currentState() const
{
    return m_player.state();
}

bool MDKPlayer::waitForState(mdk::State state, long timeout)
{
    return m_player.waitFor(state, timeout);
}

void MDKPlayer::updateRenderAPI()
{
    // Update RenderAPI with current OpenGL context if needed
    // This might be called from the render thread
    if (m_renderAPISetup) {
        m_player.setRenderAPI(&m_renderAPI);
    }
}

void MDKPlayer::updateRenderAPIWithFBO(int fboId)
{
    // Update RenderAPI with FBO ID - MDK will render directly to this FBO
    if (m_renderAPISetup) {
        m_renderAPI.fbo = fboId; // Set the FBO ID
        m_player.setRenderAPI(&m_renderAPI);
        
        if (fboId >= 0) {
            std::cout << "[MDKPlayer] RenderAPI updated with FBO ID: " << fboId << std::endl;
            std::cout.flush();
        }
    }
}

void MDKPlayer::updatePosition()
{
    int64_t currentPosition = m_player.position();
    if (currentPosition != m_lastPosition) {
        m_lastPosition = currentPosition;
        emit positionChanged();
    }
}
