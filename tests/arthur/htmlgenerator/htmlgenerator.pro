# -*- Mode: makefile -*-
COMMON_FOLDER = $$PWD/../common
include(../arthurtester.pri)
TEMPLATE = app
TARGET = htmlgenerator
DEPENDPATH += .
INCLUDEPATH += .
DESTDIR = ../bin

CONFIG += console

QT += svg xml core-private gui-private
contains(QT_CONFIG, opengl)|contains(QT_CONFIG, opengles1)|contains(QT_CONFIG, opengles2):QT += opengl

# Input
HEADERS += htmlgenerator.h
SOURCES += main.cpp htmlgenerator.cpp


