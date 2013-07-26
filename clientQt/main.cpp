#include <QGuiApplication>
#include "mainwindow.h"

int main(int argc, char* argv[])
{
    QGuiApplication app(argc,argv);

    MainWindow *mainWindow = new MainWindow();
    mainWindow->showQmlView(&app);

    return app.exec();
}
