CONFIG += testcase
TARGET = ../tst_qtextstream
QT = core network testlib
SOURCES = ../tst_qtextstream.cpp
RESOURCES += ../qtextstream.qrc

win32 {
    CONFIG(debug, debug|release) {
        TARGET = ../../debug/tst_qtextstream
    } else {
        TARGET = ../../release/tst_qtextstream
    }
}

TESTDATA += \
    ../rfc3261.txt \
    ../shift-jis.txt \
    ../task113817.txt \
    ../qtextstream.qrc \
    ../tst_qtextstream.cpp \
    ../resources

builtin_testdata {
    DEFINES += BUILTIN_TESTDATA
}
