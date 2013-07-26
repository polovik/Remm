#ifndef CONNECTION_H
#define CONNECTION_H

#include <QObject>
#include <QUdpSocket>
#include <QImage>

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

signals:
    void pictureReceived(QImage jpgImage);
    void gpsPosReceived(float Lat, float Lon);

public slots:
    void tryDirectConnectToRPi(QString address, quint16 port);
    void startCommunicate();
    void send_command(float fps);

private slots:
    void udpError(QAbstractSocket::SocketError error);
    void udpStateChanged(QAbstractSocket::SocketState state);
    void readPendingDatagram();
};

#endif // CONNECTION_H
