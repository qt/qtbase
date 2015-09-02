CONFIG += testcase
# Allow more time since examples are compiled, which may take longer on Windows.
win32:testcase.timeout=900
TARGET = tst_qmake
HEADERS += testcompiler.h
SOURCES += tst_qmake.cpp testcompiler.cpp
QT = core testlib

cross_compile: DEFINES += QMAKE_CROSS_COMPILED
debug_and_release {
    CONFIG(debug, debug|release): \
        DEFINES += DEBUG_BUILD
    else: \
        DEFINES += RELEASE_BUILD
}

TESTDATA += testdata/*
