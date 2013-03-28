CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qxml

SOURCES += tst_qxml.cpp
QT = core xml testlib

TESTDATA += 0x010D.xml
