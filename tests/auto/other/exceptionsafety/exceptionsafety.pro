CONFIG += testcase
TARGET = tst_exceptionsafety
SOURCES += tst_exceptionsafety.cpp
QT = core testlib
CONFIG += parallel_test
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
