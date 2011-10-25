CONFIG += testcase
SOURCES  += ../tst_qtextstream.cpp

TARGET = ../tst_qtextstream

win32 {
  CONFIG(debug, debug|release) {
    TARGET = ../../debug/tst_qtextstream
} else {
    TARGET = ../../release/tst_qtextstream
  }
}

RESOURCES += ../qtextstream.qrc

QT = core network testlib

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
