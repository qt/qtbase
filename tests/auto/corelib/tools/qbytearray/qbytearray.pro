CONFIG += testcase parallel_test
TARGET = tst_qbytearray
QT = core-private testlib
SOURCES = tst_qbytearray.cpp

wince* {
    addFile.files = rfc3252.txt
    addFile.path = .
    DEPLOYMENT += addFile
    DEFINES += SRCDIR=\\\"./\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}
