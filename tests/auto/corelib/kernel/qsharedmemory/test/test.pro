CONFIG += testcase

QT = core-private testlib

mac:CONFIG -= app_bundle
linux:LIBS += -lrt

SOURCES += tst_qsharedmemory.cpp
TARGET = tst_qsharedmemory

CONFIG(debug_and_release) {
    CONFIG(debug, debug|release) {
        DESTDIR = ../debug
    } else {
        DESTDIR = ../release
    }
} else {
    DESTDIR = ..
}
