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
    QString javaScript = QString("display_current_position(%1, %2)").arg(Lat).arg(Lon);
    webView->runJavaScriptInMainFrame(javaScript, NULL, NULL);
}

void MainWindow::zoomMap(int zoom)
{
    if (webView == NULL) {
        qCritical("%s::%s() Can't get access to GoogleMaps view", typeid(*this).name(), __FUNCTION__ );
        return;
    }
    QString javaScript = QString("zoomMap(%1)").arg(zoom);
    webView->runJavaScriptInMainFrame(javaScript, NULL, NULL);
}

void MainWindow::showQmlView(QGuiApplication *app)
{
    printf("INFO  %s() Enter in Main LOOP.\n", __FUNCTION__);
    qmlView = new QQuickView();
    QObject::connect(qmlView->engine(), SIGNAL(quit()), (QObject *)app, SLOT(quit()));

    cameraSource = new CameraSource(QSize(640, 480));
    connection = new Connection();
    QObject::connect(connection, SIGNAL(pictureReceived(QImage)), cameraSource, SLOT(displayFrame(QImage)));
    QObject::connect(connection, SIGNAL(gpsPosReceived(float, float)), this, SLOT(displayGPSposition(float, float)));

    QQmlContext *qmlContext = qmlView->rootContext();
    qmlContext->setContextProperty("connectRPi", (QObject *)connection);
    qmlContext->setContextProperty("mainWindow", (QObject *)this);
    qmlContext->setContextProperty("sourceCamera", (QObject *)cameraSource);
    qmlView->setResizeMode(QQuickView::SizeRootObjectToView);
    qmlView->setSource(QUrl("MainWindow.qml"));

    webView = qmlView->rootObject()->findChild<QQuickWebView *>("navigationView");
    if (webView == NULL) {
        qFatal("%s::%s() Can't get access to GoogleMaps view", typeid(*this).name(), __FUNCTION__ );
        return;
    }

    QObject *heightCanvas = qmlView->rootObject()->findChild<QObject *>("heightCanvas");
    if (heightCanvas == NULL) {
        qFatal("%s::%s() Can't get access to Height canvas", typeid(*this).name(), __FUNCTION__ );
        return;
    }
    QObject::connect(connection, SIGNAL(heightReceived(QVariant)), heightCanvas, SLOT(updateHeight(QVariant)));

    QObject *outboardDisplayCanvas = qmlView->rootObject()->findChild<QObject *>("outboardDisplayCanvas");
    if (outboardDisplayCanvas == NULL) {
        qFatal("%s::%s() Can't get access to OutBoard display canvas", typeid(*this).name(), __FUNCTION__ );
        return;
    }
    QObject::connect(connection, SIGNAL(pitchReceived(QVariant)), outboardDisplayCanvas, SLOT(updatePitch(QVariant)));
    QObject::connect(connection, SIGNAL(rollReceived(QVariant)), outboardDisplayCanvas, SLOT(updateRoll(QVariant)));

    QObject *compassCanvas = qmlView->rootObject()->findChild<QObject *>("compassCanvas");
    if (compassCanvas == NULL) {
        qFatal("%s::%s() Can't get access to Compass canvas", typeid(*this).name(), __FUNCTION__ );
        return;
    }
    QObject::connect(connection, SIGNAL(headingReceived(QVariant)), compassCanvas, SLOT(updateHeading(QVariant)));

    QObject *canvasBattery = qmlView->rootObject()->findChild<QObject *>("canvasBattery");
    if (canvasBattery == NULL) {
        qFatal("%s::%s() Can't get access to Battery level canvas", typeid(*this).name(), __FUNCTION__ );
        return;
    }
    QObject::connect(connection, SIGNAL(batteryLevelReceived(QVariant)), canvasBattery, SLOT(updateBatteryLevel(QVariant)));

    qmlView->show();
}
