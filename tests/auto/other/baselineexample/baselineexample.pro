#-------------------------------------------------
#
# Project created by QtCreator 2010-12-09T14:55:13
#
#-------------------------------------------------

QT       += testlib widgets

TARGET = tst_baselineexample
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += tst_baselineexample.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"

include($$PWD/../../../baselineserver/shared/qbaselinetest.pri)
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
