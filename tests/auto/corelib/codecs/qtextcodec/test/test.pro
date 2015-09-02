CONFIG += testcase
QT = core testlib
SOURCES = ../tst_qtextcodec.cpp

TARGET = ../tst_qtextcodec
win32 {
    CONFIG(debug, debug|release) {
        TARGET = ../../debug/tst_qtextcodec
    } else {
        TARGET = ../../release/tst_qtextcodec
    }
}
TESTDATA += ../*.txt
