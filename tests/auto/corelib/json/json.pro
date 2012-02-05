TARGET = tst_qtjson
QT = core testlib
CONFIG -= app_bundle
CONFIG += testcase

DEFINES += SRCDIR=\\\"$$PWD/\\\"

SOURCES += tst_qtjson.cpp
