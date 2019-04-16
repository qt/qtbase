CONFIG += testcase

SOURCES += tst_qocsp.cpp
QT = core network network-private testlib

TARGET = tst_qocsp

win32 {
    CONFIG(debug, debug|release) {
        DESTDIR = debug
    } else {
        DESTDIR = release
    }
}

