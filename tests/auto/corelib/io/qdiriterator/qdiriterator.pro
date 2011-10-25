CONFIG += testcase
TARGET = tst_qdiriterator
SOURCES  += tst_qdiriterator.cpp
RESOURCES      += qdiriterator.qrc
QT = core testlib

wince*mips*|wincewm50smart-msvc200*: DEFINES += WINCE_BROKEN_ITERATE=1

CONFIG += parallel_test
CONFIG += insignificant_test # QTBUG-21160
