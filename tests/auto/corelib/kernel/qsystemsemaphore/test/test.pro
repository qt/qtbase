CONFIG += testcase
QT = core testlib

win32: CONFIG += console

SOURCES += tst_qsystemsemaphore.cpp
TARGET = tst_qsystemsemaphore

CONFIG(debug_and_release) {
    CONFIG(debug, debug|release) {
        DESTDIR = ../debug
    } else {
        DESTDIR = ../release
    }
} else {
    DESTDIR = ..
}
