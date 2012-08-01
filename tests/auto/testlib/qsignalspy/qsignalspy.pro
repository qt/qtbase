CONFIG += testcase
TARGET = tst_qsignalspy
SOURCES  += tst_qsignalspy.cpp
QT = core testlib
CONFIG += parallel_test
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
