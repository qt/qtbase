CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_quuid
QT = core testlib
SOURCES = ../tst_quuid.cpp

CONFIG(debug_and_release_target) {
    CONFIG(debug, debug|release) {
        DESTDIR = ../debug
    } else {
        DESTDIR = ../release
    }
} else {
    DESTDIR = ..
}
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
