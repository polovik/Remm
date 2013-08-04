#ifndef CONNECTION_H
#define CONNECTION_H

#include <QObject>
#include <QUdpSocket>
#include <QImage>
#include <QTimer>
#include "../packet.h"

class Connection : public QObject
{
    Q_OBJECT
public:
    explicit Connection(QObject *parent = 0);
    
private:
    void data_rx(unsigned char *data, unsigned int length);
    void picture_rx(unsigned char *data, unsigned int length);
    int picture_assembly(unsigned char *data, unsigned int length);

    QUdpSocket *udpSocket;
    QTimer retransmitTimer;
    control_packet_s controlPacket;

signals:
    void pictureReceived(QImage jpgImage);
    void gpsPosReceived(float Lat, float Lon);

public slots:
    void tryDirectConnectToRPi(QString address, quint16 port, unsigned int frameWidth,
                               unsigned int frameHeight, unsigned int exposure_type,
                               unsigned int exposure_value, unsigned int quality);
    void updateCameraSettings(float fps);
    void updatePosition(int direction);
    void send_command();

private slots:
    void startCommunicate();
    void udpError(QAbstractSocket::SocketError error);
    void udpStateChanged(QAbstractSocket::SocketState state);
    void readPendingDatagram();
};

#endif // CONNECTION_H
