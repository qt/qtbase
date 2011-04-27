#-------------------------------------------------
#
# Project created by QtCreator 2010-12-09T14:55:13
#
#-------------------------------------------------

QT       += testlib

TARGET = tst_baselineexample
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += tst_baselineexample.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"

include($$PWD/../../arthur/common/qbaselinetest.pri)
