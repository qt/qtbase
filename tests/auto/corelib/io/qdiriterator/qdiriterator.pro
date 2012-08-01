CONFIG += testcase
TARGET = tst_qdiriterator
QT = core-private core testlib
SOURCES = tst_qdiriterator.cpp
RESOURCES += qdiriterator.qrc

TESTDATA += entrylist

wince*mips*|wincewm50smart-msvc200*: DEFINES += WINCE_BROKEN_ITERATE=1
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
