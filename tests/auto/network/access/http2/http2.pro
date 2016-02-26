QT += core core-private network network-private testlib

CONFIG += testcase parallel_test c++11
TEMPLATE = app
TARGET = tst_http2
HEADERS += http2srv.h
SOURCES += tst_http2.cpp http2srv.cpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"
