CONFIG += testcase
TARGET = tst_quuid
QT = core testlib
SOURCES = ../tst_quuid.cpp

darwin {
    OBJECTIVE_SOURCES = ../tst_quuid_darwin.mm
    LIBS += -framework Foundation
}

CONFIG(debug_and_release_target) {
    CONFIG(debug, debug|release) {
        DESTDIR = ../debug
    } else {
        DESTDIR = ../release
    }
} else {
    DESTDIR = ..
}
