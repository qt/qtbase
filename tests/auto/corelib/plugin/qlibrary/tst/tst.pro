CONFIG += testcase
CONFIG -= app_bundle
TARGET = ../tst_qlibrary
QT = core testlib
SOURCES = ../tst_qlibrary.cpp

win32 {
    CONFIG(debug, debug|release) {
        TARGET = ../../debug/tst_qlibrary
    } else {
        TARGET = ../../release/tst_qlibrary
    }
}

TESTDATA += ../library_path/invalid.so
