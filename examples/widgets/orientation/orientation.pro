#-------------------------------------------------
#
# Project created by QtCreator 2010-08-04T10:27:31
#
#-------------------------------------------------

QT       += core gui

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

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
maemo5: include($$QT_SOURCE_TREE/examples/maemo5pkgrules.pri)

symbian: warning(This example might not fully work on Symbian platform)
maemo5: warning(This example might not fully work on Maemo platform)
simulator: warning(This example might not fully work on Simulator platform)
