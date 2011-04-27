# -*- Mode: makefile -*-
COMMON_FOLDER = $$PWD/../common
include(../arthurtester.pri)
TEMPLATE = app
TARGET = htmlgenerator
DEPENDPATH += .
INCLUDEPATH += .
DESTDIR = ../bin

CONFIG += console

QT += svg xml
contains(QT_CONFIG, opengl)|contains(QT_CONFIG, opengles1)|contains(QT_CONFIG, opengles2):QT += opengl
contains(QT_CONFIG, qt3support):QT += qt3support

# Input
HEADERS += htmlgenerator.h
SOURCES += main.cpp htmlgenerator.cpp


