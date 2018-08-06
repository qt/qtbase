CONFIG += testcase
TARGET = tst_qsignalspy
SOURCES  += tst_qsignalspy.cpp
QT = core testlib

include($$QT_SOURCE_TREE/src/testlib/selfcover.pri)
