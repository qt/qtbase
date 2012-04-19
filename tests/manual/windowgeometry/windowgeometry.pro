QT       += core gui
TARGET = windowgeometry
TEMPLATE = app

SOURCES += main.cpp controllerwidget.cpp
HEADERS += controllerwidget.h

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
