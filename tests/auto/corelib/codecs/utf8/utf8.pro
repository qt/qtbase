CONFIG += testcase
TARGET = tst_utf8
QT = core testlib
SOURCES  += tst_utf8.cpp utf8data.cpp
CONFIG += parallel_test
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
