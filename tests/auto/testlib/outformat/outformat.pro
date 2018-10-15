CONFIG += testcase
QT = core testlib

SOURCES += tst_outformat.cpp
TARGET = outformat

include($$QT_SOURCE_TREE/src/testlib/selfcover.pri)
