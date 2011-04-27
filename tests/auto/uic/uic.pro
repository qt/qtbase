load(qttest_p4)

CONFIG += qtestlib
SOURCES += tst_uic.cpp
TARGET = tst_uic

# This test is not run on wince (I think)
DEFINES += SRCDIR=\\\"$$PWD/\\\"
