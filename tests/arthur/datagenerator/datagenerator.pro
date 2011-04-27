# -*- Mode: makefile -*-
COMMON_FOLDER = $$PWD/../common
include(../arthurtester.pri)
CONFIG += debug console
TEMPLATE = app
TARGET = datagenerator
DEPENDPATH += .
INCLUDEPATH += .
DESTDIR = ../bin

QT += svg xml
contains(QT_CONFIG, opengl)|contains(QT_CONFIG, opengles1)|contains(QT_CONFIG, opengles2):QT += opengl
contains(QT_CONFIG, qt3support):QT += qt3support

# Input
HEADERS += datagenerator.h \
	xmlgenerator.h
SOURCES += main.cpp datagenerator.cpp \
	xmlgenerator.cpp 

DEFINES += QT_USE_USING_NAMESPACE

