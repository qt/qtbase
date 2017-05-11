TEMPLATE = app

TARGET = app
QT = core

DESTDIR = ./

CONFIG -= app_bundle
CONFIG += console

SOURCES += main.cpp
DEFINES += QT_MESSAGELOGCONTEXT

gcc:!mingw:!haiku {
    QMAKE_LFLAGS += -rdynamic
    contains(QT_ARCH, arm): QMAKE_CXXFLAGS += -funwind-tables -fno-inline
}
