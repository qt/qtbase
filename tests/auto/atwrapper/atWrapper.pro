# -*- Mode: makefile -*-

ARTHUR=$$QT_SOURCE_TREE/tests/arthur
COMMON_FOLDER = $$ARTHUR/common
include($$ARTHUR/arthurtester.pri)
TEMPLATE = app
INCLUDEPATH += $$ARTHUR
DEFINES += SRCDIR=\\\"$$PWD\\\"

QT += xml svg network

contains(QT_CONFIG, qt3support): QT += qt3support
contains(QT_CONFIG, opengl):QT += opengl

include($$ARTHUR/datagenerator/datagenerator.pri)

load(qttest_p4)

# Input
HEADERS += atWrapper.h
SOURCES += atWrapperAutotest.cpp atWrapper.cpp

TARGET = tst_atwrapper

#include($$COMMON_FOLDER/common.pri)
