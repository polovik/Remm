#include <QDebug>
#include <typeinfo>
#include "../packet.h"
#include "connection.h"

using namespace std;
static const int RETRANSMIT_COMMAND_INTERVAL = 1000;    //  in msec

Connection::Connection(QObject *parent) :
    QObject(parent)
{
    udpSocket = new QUdpSocket(this);
    connect(udpSocket, SIGNAL(readyRead()), this, SLOT(readPendingDatagram()));
    connect(udpSocket, SIGNAL(connected()), this, SLOT(startCommunicate()));
    connect(udpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(udpError(QAbstractSocket::SocketError)));
    connect(udpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            this, SLOT(udpStateChanged(QAbstractSocket::SocketState)));

    retransmitTimer.setInterval(RETRANSMIT_COMMAND_INTERVAL);
    connect(&retransmitTimer, SIGNAL(timeout()), this, SLOT(send_command()));

    memset(&controlPacket, 0x00, sizeof(control_packet_s));
}

void Connection::tryDirectConnectToRPi(QString address, quint16 port, unsigned int frameWidth,
                                       unsigned int frameHeight, unsigned int exposure_type,
                                       unsigned int exposure_value, unsigned int quality)
{
    qDebug() << typeid(*this).name() << ":" << __FUNCTION__ << "()"
             << frameWidth << frameHeight << exposure_type << exposure_value << quality;
    udpSocket->close();

    memset(&controlPacket, 0x00, sizeof(control_packet_s));
    controlPacket.magic = MAGIC_COMMAND;
    controlPacket.height = 0;
    controlPacket.direction = 0;
    controlPacket.gps_latitude = 0.0;
    controlPacket.gps_longitude = 0.0;
    controlPacket.slope = 0;
    controlPacket.camera.width = frameWidth;
    controlPacket.camera.height = frameHeight;
    controlPacket.camera.quality = quality;
    controlPacket.camera.exposure_type = exposure_type;
    controlPacket.camera.exposure_value = exposure_value;
    controlPacket.camera.fps = 1.0;
    controlPacket.command = AUTOPILOT_OFF;

    udpSocket->connectToHost(address, port);
}

void Connection::updateCameraSettings(float fps)
{
    controlPacket.camera.fps = fps;
    send_command();
}

void Connection::updatePosition(int direction)
{
    controlPacket.direction = direction;
    send_command();
}

void Connection::startCommunicate()
{
    qDebug() << typeid(*this).name() << ":" << __FUNCTION__ << "()";
    udpSocket->write("Hello!");

    retransmitTimer.start();
//    qDebug() << __PRETTY_FUNCTION__;
}

void Connection::udpError(QAbstractSocket::SocketError error)
{
    qDebug() << typeid(*this).name() << ":" << __FUNCTION__ << "()"
             << "Error #" << error << " has occured." << udpSocket->errorString();
}

void Connection::udpStateChanged(QAbstractSocket::SocketState state)
{
    qDebug() << typeid(*this).name() << ":" << __FUNCTION__ << "()"
             << "Current state is " << state;
    if ((state == QAbstractSocket::ClosingState) || (state == QAbstractSocket::UnconnectedState))
        retransmitTimer.stop();
}

static status_packet_s last_status;

void Connection::data_rx(unsigned char *data, unsigned int length)
{
    status_packet_s *status_packet;

    if (length != sizeof(status_packet_s)) {
        qDebug("ERROR %s() Unexpected packet size %d: %.*s", __FUNCTION__, length, length, data);
        return;
    }
    status_packet = (status_packet_s *)data;
    if (status_packet->magic != MAGIC_STATUS) {
        qDebug("ERROR %s() Unexpected packet type 0x%08X: %.*s", __FUNCTION__, status_packet->magic, length, data);
        return;
    }

    memcpy(&last_status, status_packet, sizeof(status_packet_s));
//    qDebug("INFO  %s() height=%d, direction=%d, gps_latitude=%f, gps_longitude=%f, "
//            "slope=%d, battery_charge=%d, info=%s.", __FUNCTION__,
//            status_packet->height, status_packet->direction, status_packet->gps_latitude,
//            status_packet->gps_longitude, status_packet->slope, status_packet->battery_charge, status_packet->info);
    emit gpsPosReceived(status_packet->gps_latitude, status_packet->gps_longitude);
}

void Connection::picture_rx(unsigned char *data, unsigned int length)
{
//    qDebug("INFO  %s() Picture has just received.", __FUNCTION__);

    if ((length == 0) || (data == NULL)) {
        qDebug("ERROR %s() Incorrect arguments data=0x%X, length=%d", __FUNCTION__, (unsigned int)data, length);
        return;
    }

    QImage jpgImage;
    jpgImage.loadFromData(data, length, "JPG");
    emit pictureReceived(jpgImage);

//    char filename[] = "captured.jpg";
//    FILE *file = fopen(filename, "wb");
//    fwrite(data, 1, length, file);
//    fclose(file);
}

/**	Check for picture inside packet.
 *  Return 1 if picture has found. Otherwise return 0.
 */
int Connection::picture_assembly(unsigned char *data, unsigned int length)
{
    static picture_packet_s prev_packet = { /*magic*/ MAGIC_STATUS, /*picture_id*/ -1, /*picture_size*/ 0, /*fragment_id*/ 0, /*fragment_size*/ 0, /*data*/ "" } ;
    static unsigned char *picture;
    picture_packet_s *packet = (picture_packet_s *)data;

    Q_UNUSED(length);
    if (packet->magic != MAGIC_PICTURE)
        return 0;

    if (packet->picture_id < prev_packet.picture_id) {
        qDebug("INFO  %s() Received frame %d for old picture %d. Skip it.",
               __FUNCTION__, packet->picture_id, prev_packet.picture_id);
        return 1;
    }
    if (packet->picture_id > prev_packet.picture_id) {
        picture_rx(picture, prev_packet.picture_size);
        free(picture);
//        qDebug("INFO  %s() Start receiving new picture %d.", __FUNCTION__, packet->picture_id);
        prev_packet = *packet;
        picture = (unsigned char *)malloc(packet->picture_size);
    }
    int dest_pos = packet->fragment_id * FRAGMENT_SIZE;
    if ((dest_pos + packet->fragment_size) > packet->picture_size) {
        qDebug("ERROR %s() Incorrect packet - May occur \"Out of memory\"."
               "Allocated memory size = %dbytes, request %dbytes. fragment_id=%d, fragment_size=%d.",
               __FUNCTION__, packet->picture_size, dest_pos + packet->fragment_size, packet->fragment_id, packet->fragment_size);
        return 1;
    }
    memcpy(&picture[dest_pos], packet->data, packet->fragment_size);
    return 1;
}

void Connection::readPendingDatagram()
{
    unsigned char *data;
    unsigned int length = udpSocket->bytesAvailable();
//    qDebug() << typeid(*this).name() << ":" << __FUNCTION__ << "()"
//             << "Received" << length << "bytes";
    data = new (nothrow) unsigned char[length];
    if (data == NULL) {
        qDebug() << typeid(*this).name() << ":" << __FUNCTION__ << "()"
                 << "Can't allocate" << length << "bytes for datagramm";
        return;
    }
    int readed = udpSocket->read((char *)data, length);
    if (readed != (int)length) {
        qDebug() << typeid(*this).name() << ":" << __FUNCTION__ << "()"
                 << "Can't read all data. Received" << length << "bytes, readed -" << readed;
        delete[] data;
        return;
    }

    if (picture_assembly(data, length) == 1)
        return;

    data_rx(data, length);
    delete[] data;
}

void Connection::send_command()
{
    int sended = udpSocket->write((char *)&controlPacket, sizeof(control_packet_s));
    if (sended != sizeof(control_packet_s)) {
        qDebug() << typeid(*this).name() << ":" << __FUNCTION__ << "()"
                 << "Sended only" << sended << "bytes from" << sizeof(control_packet_s);
    }
}
