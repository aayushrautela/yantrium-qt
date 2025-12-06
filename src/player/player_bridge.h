#ifndef PLAYER_BRIDGE_H
#define PLAYER_BRIDGE_H

#include <QQuickFramebufferObject>
#include <QQuickWindow>
#include <QTimer>
#include <memory>
#include <QtQmlIntegration/qqmlintegration.h>
#include "mdk_player.h"

class PlayerRenderer;

class PlayerBridge : public QQuickFramebufferObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)
    Q_PROPERTY(int64_t duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(int64_t position READ position NOTIFY positionChanged)
    
public:
    explicit PlayerBridge(QQuickItem *parent = nullptr);
    ~PlayerBridge();
    
    // Property getters
    QString source() const { return m_source; }
    void setSource(const QString &source);
    
    bool isPlaying() const;
    int64_t duration() const;
    int64_t position() const;
    
    // Q_INVOKABLE methods
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void seek(int64_t ms);
    
signals:
    void sourceChanged();
    void isPlayingChanged();
    void durationChanged();
    void positionChanged();

protected:
    Renderer *createRenderer() const override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    
private slots:
    void handleWindowChanged(QQuickWindow *win);
    
private:
    std::unique_ptr<MDKPlayer> m_player;
    std::unique_ptr<QTimer> m_updateTimer;
    QString m_source;
    bool m_isPlaying;
};

#endif // PLAYER_BRIDGE_H
