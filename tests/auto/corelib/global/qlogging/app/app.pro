TEMPLATE = app

TARGET = app
QT = core

DESTDIR = ./

CONFIG -= app_bundle
CONFIG += console

SOURCES += main.cpp
DEFINES += QT_MESSAGELOGCONTEXT

gcc:!mingw:!haiku: QMAKE_LFLAGS += -rdynamic

