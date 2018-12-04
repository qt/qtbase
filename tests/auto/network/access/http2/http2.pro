QT = core core-private network network-private testlib

CONFIG += testcase parallel_test c++11
TARGET = tst_http2
INCLUDEPATH += ../../../../shared/
HEADERS += http2srv.h ../../../../shared/emulationdetector.h
SOURCES += tst_http2.cpp http2srv.cpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"
