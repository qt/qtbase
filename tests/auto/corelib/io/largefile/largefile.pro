CONFIG += testcase
TARGET = tst_largefile
QT = core testlib
SOURCES = tst_largefile.cpp

wince: SOURCES += $$QT_SOURCE_TREE/src/corelib/kernel/qfunctions_wince.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
