QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TARGET = windowchildgeometry
TEMPLATE = app

INCLUDEPATH += ../windowflags
SOURCES += $$PWD/main.cpp controllerwidget.cpp ../windowflags/controls.cpp
HEADERS += controllerwidget.h ../windowflags/controls.h

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
