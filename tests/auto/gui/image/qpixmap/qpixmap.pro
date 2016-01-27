CONFIG += testcase
TARGET = tst_qpixmap

QT += core-private gui-private testlib
qtHaveModule(widgets): QT += widgets widgets-private

SOURCES  += tst_qpixmap.cpp
!wince:!winrt {
   win32:LIBS += -lgdi32 -luser32
}

RESOURCES += qpixmap.qrc
TESTDATA += convertFromImage/* convertFromToHICON/* loadFromData/* images/*
