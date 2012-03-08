CONFIG += testcase
TARGET = tst_qpixmap

QT += core-private gui-private widgets widgets-private testlib

SOURCES  += tst_qpixmap.cpp
!wince* {
   win32:LIBS += -lgdi32 -luser32
}

RESOURCES += qpixmap.qrc
TESTDATA += convertFromImage/* convertFromToHICON/* loadFromData/* images/*
