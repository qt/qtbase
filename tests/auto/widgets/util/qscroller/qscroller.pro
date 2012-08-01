CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qscroller

QT += widgets testlib
SOURCES += tst_qscroller.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
