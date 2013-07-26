#include <QObject>
#include <QDir>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QQuickView>
#include <QQmlContext>
#include "connection.h"
#include "camerasource.h"
#include "mainwindow.h"

QQmlContext *context = NULL;
CameraSource *cameraSource = NULL;
Connection *connection = NULL;
MainWindow *mainWindow = NULL;

//static int connection_established = 0;

//void start_communicate()
//{
//    printf("INFO  %s() Connection is established.\n", __FUNCTION__);
//    connection_established = 1;
//}

void destroy_connection(int signum)
{
    printf("INFO  %s() Destroy connection. signum=%d\n", __FUNCTION__, signum);
    exit(0);
}

int main(int argc, char* argv[])
{
    QGuiApplication app(argc,argv);
    QQuickView view;
    view.connect(view.engine(), SIGNAL(quit()), &app, SLOT(quit()));

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
    mainWindow = new MainWindow();
    cameraSource = new CameraSource(QSize(640, 480));
    connection = new Connection();
    QObject::connect(connection, SIGNAL(pictureReceived(QImage)), cameraSource, SLOT(displayFrame(QImage)));

    context = view.rootContext();
//    view.rootContext()->setContextProperty("mainWindow", (QObject *)mainWindow);
    view.rootContext()->setContextProperty("connectRPi", (QObject *)connection);
    context->setContextProperty("sourceCamera", (QObject *)cameraSource);
    view.setSource(QUrl("mainWindow.qml"));
    view.show();

    return app.exec();
}
