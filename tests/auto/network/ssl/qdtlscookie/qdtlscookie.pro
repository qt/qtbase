CONFIG += testcase

SOURCES += tst_qdtlscookie.cpp
QT = core network-private testlib

TARGET = tst_qdtlscookie

win32 {
    CONFIG(debug, debug|release) {
        DESTDIR = debug
    } else {
        DESTDIR = release
    }
}

