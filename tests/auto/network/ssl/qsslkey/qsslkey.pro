CONFIG += testcase
CONFIG += parallel_test

SOURCES += tst_qsslkey.cpp
!wince*:win32:LIBS += -lws2_32
QT = core network testlib

TARGET = tst_qsslkey

TESTDATA += keys/* rsa-without-passphrase.pem rsa-with-passphrase.pem
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
