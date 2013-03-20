CONFIG += testcase parallel_test
TARGET = tst_qmessageauthenticationcode
QT = core testlib
SOURCES = tst_qmessageauthenticationcode.cpp

TESTDATA += data/*
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
