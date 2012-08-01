CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qkeysequence

QT += testlib
QT += core-private gui-private

SOURCES  += tst_qkeysequence.cpp

RESOURCES += qkeysequence.qrc
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
