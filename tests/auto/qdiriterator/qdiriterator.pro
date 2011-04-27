load(qttest_p4)
SOURCES  += tst_qdiriterator.cpp
RESOURCES      += qdiriterator.qrc
QT = core

wince*mips*|wincewm50smart-msvc200*: DEFINES += WINCE_BROKEN_ITERATE=1

CONFIG += parallel_test
