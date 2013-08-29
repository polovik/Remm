#ifndef CAMERASOURCE_H
#define CAMERASOURCE_H

#include <QObject>
#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>

class CameraSource : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractVideoSurface *videoSurface WRITE setVideoSurface)
public:
    explicit CameraSource(QSize size, QObject *parent = 0);
    
signals:

private:
    QAbstractVideoSurface *m_videoSurface;
    QVideoSurfaceFormat m_format;
    QSize m_size;
    
public Q_SLOTS:
    void setVideoSurface(QAbstractVideoSurface *surface);
    void displayFrame(QImage jpgImage);
};

#endif // CAMERASOURCE_H
