CONFIG += testcase
QT += testlib
SOURCES = ../tst_qtextcodec.cpp

!wince* {
    TARGET = ../tst_qtextcodec
    win32: {
        CONFIG(debug, debug|release) {
            TARGET = ../../debug/tst_qtextcodec
        } else {
            TARGET = ../../release/tst_qtextcodec
        }
    }
    DEFINES += SRCDIR=\\\"$$PWD/../\\\"
} else {
    TARGET = tst_qtextcodec
    addFiles.files = ../*.txt
    addFiles.path = .
    DEPLOYMENT += addFiles
    qt_not_deployed {
        DEPLOYMENT_PLUGIN += qcncodecs qjpcodecs qkrcodecs qtwcodecs
    }
    DEFINES += SRCDIR=\\\"\\\"
}
