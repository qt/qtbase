load(qttest_p4)
SOURCES += tst_qtransformedscreen.cpp

embedded:!contains(gfx-drivers, transformed) {
LIBS += ../../../plugins/gfxdrivers/libqgfxtransformed.so
}

