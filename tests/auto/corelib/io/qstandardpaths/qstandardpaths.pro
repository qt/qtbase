TARGET = tst_qstandardpaths
CONFIG += testcase
SOURCES += tst_qstandardpaths.cpp
QT = core testlib
CONFIG += parallel_test

wince* {
    DEFINES += SRCDIR=\\\"\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

