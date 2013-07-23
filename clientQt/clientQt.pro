TEMPLATE = app

QT += quick qml multimedia

SOURCES += main.cpp camerasource.cpp ../connection.c ../http.c
HEADERS += camerasource.h ../connection.h ../http.h ../packet.h
DEFINES += CLIENT_SIDE=1

HOST_PJ = ../../pjproject-i686
HOST_ARCH = i686-pc-linux-gnu
INCLUDEPATH += $$HOST_PJ/pjlib/include $$HOST_PJ/pjnath/include $$HOST_PJ/pjlib-util/include
LIBS += -L$$HOST_PJ/pjlib/lib -L$$HOST_PJ/pjlib-util/lib -L$$HOST_PJ/pjnath/lib -lpjnath-$$HOST_ARCH -lpjlib-util-$$HOST_ARCH -lpj-$$HOST_ARCH
QMAKE_CC = g++

#HOST_LDFLAGS=-lpthread -ldl
#HOST_LIBS= -lm -lnsl -lrt -lpthread
#HOST_CFLAGS=-g -Werror -Wall

RESOURCES +=

OTHER_FILES = mainWindow.qml google_maps.html ../connect.cgi ../README.md

