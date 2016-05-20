QT       += core network

# gui needed for QImage
# QT       -= gui

TARGET = baselineserver
DESTDIR = ../bin
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

include(../shared/baselineprotocol.pri)

SOURCES += main.cpp \
    baselineserver.cpp \
    report.cpp

HEADERS += \
    baselineserver.h \
    report.h

RESOURCES += \
    baselineserver.qrc
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
