# -*- Mode: makefile -*-
COMMON_FOLDER = $$PWD/../common
include(../arthurtester.pri)
TEMPLATE = app
TARGET = performancediff
DEPENDPATH += .
INCLUDEPATH += .
DESTDIR = ../bin

CONFIG += console

QT += xml svg
contains(QT_CONFIG, opengl)|contains(QT_CONFIG, opengles1)|contains(QT_CONFIG, opengles2):QT += opengl
contains(QT_CONFIG, qt3support):QT += qt3support

# Input
HEADERS += performancediff.h
SOURCES += main.cpp performancediff.cpp


