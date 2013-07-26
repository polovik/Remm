#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QObject>

class QQuickView;
class QQuickWebView;
class CameraSource;
class Connection;
class QGuiApplication;

class MainWindow : public QObject
{
    Q_OBJECT
public:
    explicit MainWindow(QObject *parent = 0);
    void showQmlView(QGuiApplication *app);

private:
    QQuickView *qmlView;
    QQuickWebView *webView;
    CameraSource *cameraSource;
    Connection *connection;

signals:
    
public slots:
    void displayGPSposition(float Lat, float Lon);
};

#endif // MAINWINDOW_H
