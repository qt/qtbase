CONFIG += testcase
TARGET = tst_qxmlstream
SOURCES += tst_qxmlstream.cpp

QT = core xml network testlib

wince* {
    addFiles.files = data XML-Test-Suite
    addFiles.path = .
    DEPLOYMENT += addFiles
    wince*:DEFINES += SRCDIR=\\\"\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}
