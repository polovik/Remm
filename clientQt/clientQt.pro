TEMPLATE = app

QT += quick qml multimedia network

SOURCES += main.cpp camerasource.cpp connection.cpp \
    mainwindow.cpp
HEADERS += camerasource.h ../packet.h connection.h \
    mainwindow.h

RESOURCES +=

OTHER_FILES = mainWindow.qml google_maps.html ../connect.cgi ../README.md

