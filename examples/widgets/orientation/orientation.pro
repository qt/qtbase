#-------------------------------------------------
#
# Project created by QtCreator 2010-08-04T10:27:31
#
#-------------------------------------------------

QT       += core gui widgets

TARGET = orientation
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS += \
    portrait.ui \
    landscape.ui

RESOURCES += \
    images.qrc

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example

symbian: warning(This example might not fully work on Symbian platform)
maemo5: warning(This example might not fully work on Maemo platform)
simulator: warning(This example might not fully work on Simulator platform)
