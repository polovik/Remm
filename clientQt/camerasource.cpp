#include <QDebug>
#include "camerasource.h"

CameraSource::CameraSource(QSize size, QObject *parent) :
    QObject(parent)
{
    m_size = size;
    m_format = 	QVideoSurfaceFormat(m_size, QVideoFrame::Format_RGB32, QAbstractVideoBuffer::NoHandle);
}

void CameraSource::setVideoSurface(QAbstractVideoSurface *surface)
{
    m_videoSurface = surface;
    m_videoSurface->start(m_format);
    QImage image(m_size, QImage::Format_RGB32);
    image.fill(Qt::darkGreen);
    QVideoFrame frame(image);
    m_videoSurface->present(frame);
}

void CameraSource::displayFrame(unsigned char *data, unsigned int length)
{
    QImage jpgImage;
    jpgImage.loadFromData(data, length, "JPG");

    QVideoFrame frame(jpgImage.scaled(m_size));
    m_videoSurface->present(frame);
}
