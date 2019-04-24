CONFIG += testcase
TARGET = tst_qpixmap

QT += core-private gui-private testlib
qtHaveModule(widgets): QT += widgets widgets-private

SOURCES  += tst_qpixmap.cpp
win32:!winrt: QMAKE_USE += user32 gdi32

RESOURCES += qpixmap.qrc
TESTDATA += convertFromImage/* convertFromToHICON/* loadFromData/* images/*
