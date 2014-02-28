TARGET = tst_qarraydata
SOURCES  += $$PWD/tst_qarraydata.cpp
HEADERS  += $$PWD/simplevector.h
QT = core testlib
CONFIG += testcase parallel_test
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
