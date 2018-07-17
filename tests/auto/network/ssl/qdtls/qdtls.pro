CONFIG += testcase

SOURCES += tst_qdtls.cpp
QT = core network-private testlib

TARGET = tst_qdtls

win32 {
    CONFIG(debug, debug|release) {
        DESTDIR = debug
    } else {
        DESTDIR = release
    }
}

TESTDATA += certs
