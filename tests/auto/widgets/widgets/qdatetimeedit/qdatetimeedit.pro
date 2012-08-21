CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qdatetimeedit
QT += widgets testlib core-private widgets-private
SOURCES  += tst_qdatetimeedit.cpp

wincewm50smart-msvc2005: DEFINES += WINCE_NO_MODIFIER_KEYS
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
