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

wince* {
    addFiles.files = ../rfc3261.txt ../shift-jis.txt ../task113817.txt ../qtextstream.qrc ../tst_qtextstream.cpp
    addFiles.path = .
    res.files = ../resources
    res.path = .
    DEPLOYMENT += addFiles
    DEFINES += SRCDIR=\\\"\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/../\\\"
}

mac: CONFIG += insignificant_test # QTBUG-22767
