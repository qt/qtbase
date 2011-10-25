CONFIG += testcase
TARGET = tst_qtransformedscreen
SOURCES += tst_qtransformedscreen.cpp
QT += testlib

embedded:!contains(gfx-drivers, transformed) {
LIBS += ../../../plugins/gfxdrivers/libqgfxtransformed.so
}

