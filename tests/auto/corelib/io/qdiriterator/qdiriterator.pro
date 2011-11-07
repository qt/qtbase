CONFIG += testcase parallel_test
TARGET = tst_qdiriterator
QT = core testlib
SOURCES = tst_qdiriterator.cpp
RESOURCES += qdiriterator.qrc

wince*mips*|wincewm50smart-msvc200*: DEFINES += WINCE_BROKEN_ITERATE=1
