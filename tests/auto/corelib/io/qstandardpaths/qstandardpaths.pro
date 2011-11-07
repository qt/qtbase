CONFIG += testcase parallel_test
TARGET = tst_qstandardpaths
QT = core testlib
SOURCES = tst_qstandardpaths.cpp

wince* {
    DEFINES += SRCDIR=\\\"\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

