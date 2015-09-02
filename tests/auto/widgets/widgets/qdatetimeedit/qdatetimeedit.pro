CONFIG += testcase
TARGET = tst_qdatetimeedit
QT += widgets testlib core-private widgets-private
SOURCES  += tst_qdatetimeedit.cpp

wincewm50smart-msvc2005: DEFINES += WINCE_NO_MODIFIER_KEYS
