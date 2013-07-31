TEMPLATE = app

QT += quick qml multimedia network webkit

INCLUDEPATH += "/opt/Qt5.1.0/5.1.0/gcc/include/QtWebKit/5.1.0/QtWebKit"
INCLUDEPATH += "/opt/Qt5.1.0/5.1.0/gcc/include/QtQuick/5.1.0/QtQuick"

SOURCES += main.cpp camerasource.cpp connection.cpp \
    mainwindow.cpp
HEADERS += camerasource.h ../packet.h connection.h \
    mainwindow.h

RESOURCES +=

OTHER_FILES = google_maps.html ../connect.cgi ../README.md \
    Settings.qml \
    ControlPanel.qml \
    MainWindow.qml

