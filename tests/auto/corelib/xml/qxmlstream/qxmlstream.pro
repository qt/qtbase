CONFIG += testcase
TARGET = tst_qxmlstream
QT = core xml network testlib
SOURCES = tst_qxmlstream.cpp

wince* {
    addFiles.files = data XML-Test-Suite
    addFiles.path = .
    DEPLOYMENT += addFiles
    DEFINES += SRCDIR=\\\"\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}
