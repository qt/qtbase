# -*- Mode: makefile -*-
COMMON_FOLDER = $$PWD/../common
include(../arthurtester.pri)
TEMPLATE = app
TARGET = shower
DEPENDPATH += .
INCLUDEPATH += .
DESTDIR = ../bin

QT += xml svg core-private gui-private
contains(QT_CONFIG, opengl)|contains(QT_CONFIG, opengles1)|contains(QT_CONFIG, opengles2):QT += opengl

# Input
HEADERS += shower.h
SOURCES += main.cpp shower.cpp


