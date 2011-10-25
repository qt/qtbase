CONFIG += testcase
TARGET = tst_networkselftest

SOURCES += tst_networkselftest.cpp
QT = core network testlib

wince*: {
    addFiles.files = rfc3252.txt
    addFiles.path = .
    DEPLOYMENT += addFiles
    DEFINES += SRCDIR=\\\"\\\"
} else:vxworks*: {
    DEFINES += SRCDIR=\\\"\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

