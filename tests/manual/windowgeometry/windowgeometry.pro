QT       += core gui widgets
TARGET = windowgeometry
TEMPLATE = app

INCLUDEPATH += ../windowflags
SOURCES += $$PWD/main.cpp controllerwidget.cpp ../windowflags/controls.cpp
HEADERS += controllerwidget.h ../windowflags/controls.h

