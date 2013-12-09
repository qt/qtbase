CONFIG += testcase console
CONFIG += parallel_test
TARGET = tst_qsignalblocker
QT = core testlib
SOURCES = tst_qsignalblocker.cpp

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
