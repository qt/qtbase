CONFIG += testcase
TARGET = tst_networkselftest

SOURCES += tst_networkselftest.cpp
QT = core core-private network testlib

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
