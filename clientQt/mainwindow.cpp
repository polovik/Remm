#include <QQmlEngine>
#include <QQuickView>
#include <QQmlContext>
#include <typeinfo>
#include <private/qquickwebview_p.h>
#include "mainwindow.h"
#include "connection.h"
#include "camerasource.h"

MainWindow::MainWindow(QObject *parent) :
    QObject(parent)
{
    qmlView = NULL;
    webView = NULL;
    cameraSource = NULL;
    connection = NULL;
}

void MainWindow::displayGPSposition(float Lat, float Lon)
{
    if (webView == NULL) {
        qCritical("%s::%s() Can't get access to GoogleMaps view", typeid(*this).name(), __FUNCTION__ );
        return;
    }
    qDebug("%s::%s() Lat:%f, Lon:%f", typeid(*this).name(), __FUNCTION__ , Lat, Lon);
    QString javaScript = QString("display_current_position(%1, %2)").arg(Lat).arg(Lon);
    webView->runJavaScriptInMainFrame(javaScript, NULL, NULL);
}

void MainWindow::showQmlView(QGuiApplication *app)
{
    printf("INFO  %s() Enter in Main LOOP.\n", __FUNCTION__);
    qmlView = new QQuickView();
    QObject::connect(qmlView->engine(), SIGNAL(quit()), (QObject *)app, SLOT(quit()));
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
    cameraSource = new CameraSource(QSize(640, 480));
    connection = new Connection();
    QObject::connect(connection, SIGNAL(pictureReceived(QImage)), cameraSource, SLOT(displayFrame(QImage)));
    QObject::connect(connection, SIGNAL(gpsPosReceived(float, float)), this, SLOT(displayGPSposition(float, float)));

    QQmlContext *qmlContext = qmlView->rootContext();
    qmlContext->setContextProperty("connectRPi", (QObject *)connection);
    qmlContext->setContextProperty("sourceCamera", (QObject *)cameraSource);
    qmlView->setSource(QUrl("mainWindow.qml"));

    webView = qmlView->rootObject()->findChild<QQuickWebView *>("navigationView");
    if (webView == NULL) {
        qFatal("%s::%s() Can't get access to GoogleMaps view", typeid(*this).name(), __FUNCTION__ );
        return;
    }

    qmlView->show();
}
