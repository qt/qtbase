CONFIG += testcase
CONFIG += parallel_test

SOURCES += tst_qsslcertificate.cpp
!wince*:win32:LIBS += -lws2_32
QT = core network testlib

TARGET = tst_qsslcertificate
DEFINES += SRCDIR=\\\"$$PWD/\\\"

TESTDATA += certificates/* more-certificates/* verify-certs/* pkcs12/*
