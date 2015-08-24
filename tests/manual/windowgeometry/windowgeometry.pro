QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TARGET = windowgeometry
TEMPLATE = app

INCLUDEPATH += ../windowflags
SOURCES += $$PWD/main.cpp controllerwidget.cpp ../windowflags/controls.cpp
HEADERS += controllerwidget.h ../windowflags/controls.h

