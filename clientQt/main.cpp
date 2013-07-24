#include <signal.h>
#include <QObject>
#include <QDir>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QQuickView>
#include <QQmlContext>
#include "../connection.h"
#include "../packet.h"
#include "camerasource.h"

QQmlContext *context = NULL;
CameraSource *cameraSource = NULL;

class CallableClass : public QObject
{
    Q_OBJECT

public:
    CallableClass(QObject *parent = 0);
    virtual ~CallableClass() {}

public slots:
    void cppMethod();
};

CallableClass::CallableClass(QObject *parent) : QObject(parent)
{
}

void CallableClass::cppMethod()
{
    qDebug("C++ method called!");
    context->setContextProperty("capturedFrame", QString("next.png"));
}

static int connection_established = 0;
static status_packet_s last_status;

void start_communicate()
{
    printf("INFO  %s() Connection is established.\n", __FUNCTION__);
    connection_established = 1;
}

void data_rx(unsigned char *data, unsigned int length)
{
    status_packet_s *status_packet;

    if (length != sizeof(status_packet_s)) {
        printf("ERROR %s() Unexpected packet size %d: %.*s\n", __FUNCTION__, length, length, data);
        return;
    }
    status_packet = (status_packet_s *)data;
    if (status_packet->magic != MAGIC_STATUS) {
        printf("ERROR %s() Unexpected packet type %d: %.*s\n", __FUNCTION__, status_packet->magic, length, data);
        return;
    }

    memcpy(&last_status, status_packet, sizeof(status_packet_s));
    printf("INFO  %s() height=%d, direction=%d, gps_latitude=%f, gps_longitude=%f, "
            "slope=%d, battery_charge=%d, info=%s.\n", __FUNCTION__,
            status_packet->height, status_packet->direction, status_packet->gps_latitude,
            status_packet->gps_longitude, status_packet->slope, status_packet->battery_charge, status_packet->info);
}

void picture_rx(unsigned char *data, unsigned int length)
{
    printf("INFO  %s() Picture has just received.\n", __FUNCTION__);

    if ((length == 0) || (data == NULL)) {
        printf("ERROR %s() Incorrect arguments data=0x%X, length=%d\n", __FUNCTION__, (unsigned int)data, length);
        return;
    }

    if (cameraSource != NULL)
        cameraSource->displayFrame(data, length);

    char filename[] = "captured.jpg";
    FILE *file = fopen(filename, "wb");
    fwrite(data, 1, length, file);
    fclose(file);
}

void destroy_connection(int signum)
{
    printf("INFO  %s() Destroy connection. signum=%d\n", __FUNCTION__, signum);
//	release_display();
    icedemo_destroy_instance(signum);
    exit(0);
}

int main(int argc, char* argv[])
{
    QGuiApplication app(argc,argv);
    QQuickView view;
    view.connect(view.engine(), SIGNAL(quit()), &app, SLOT(quit()));

    // Register signal and signal handler
    signal(SIGINT, destroy_connection);

    if (start_connecting(SIDE_CLIENT) != 0)
        destroy_connection(0);  //  Exit

    printf("INFO  %s() Enter in Main LOOP.\n", __FUNCTION__);
/*    while (1) {
        if (connection_established == 1) {
            display_frame(get_fps(), last_status.battery_charge, last_status.gps_latitude,
                          last_status.gps_longitude);
            //	Poll every 100ms
            poll_keys(100);
            send_command();
        } else {
            sleep(1);
        }
    }
    printf("INFO  %s() Exit from Main LOOP.\n", __FUNCTION__);
*/
//    CallableClass *cl = new CallableClass();
    cameraSource = new CameraSource(QSize(640, 480));

    context = view.rootContext();
//    view.rootContext()->setContextProperty("cppObject", (QObject*)cl);
    context->setContextProperty("sourceCamera", (QObject *)cameraSource);
    view.setSource(QUrl("mainWindow.qml"));
    view.show();

    return app.exec();
}

#include "main.moc"
