CONFIG += testcase
TARGET = tst_qdiriterator
QT = core-private core testlib
SOURCES = tst_qdiriterator.cpp
RESOURCES += qdiriterator.qrc

TESTDATA += entrylist

wince*mips*|wincewm50smart-msvc200*: DEFINES += WINCE_BROKEN_ITERATE=1

win32: CONFIG += insignificant_test # Crashes on Windows in release builds
