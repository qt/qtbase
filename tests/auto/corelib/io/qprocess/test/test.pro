CONFIG += testcase
QT = core testlib network
embedded: QT += gui
SOURCES = ../tst_qprocess.cpp

TARGET = ../tst_qprocess
win32 {
    CONFIG(debug, debug|release) {
        TARGET = ../../debug/tst_qprocess
    } else {
        TARGET = ../../release/tst_qprocess
    }
}

TESTDATA += ../testBatFiles/*
