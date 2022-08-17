QT       += core gui widgets
TARGET = windowchildgeometry
TEMPLATE = app

INCLUDEPATH += ../windowflags
SOURCES += $$PWD/main.cpp controllerwidget.cpp ../windowflags/controls.cpp
HEADERS += controllerwidget.h ../windowflags/controls.h
