CONFIG += testcase

QT = core testlib
SOURCES += tst_uic.cpp
TARGET = tst_uic

# This test is not run on wince (I think)
DEFINES += SRCDIR=\\\"$$PWD/\\\"
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
