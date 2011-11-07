CONFIG += testcase parallel_test
TARGET = tst_qelapsedtimer
QT = core testlib
SOURCES = tst_qelapsedtimer.cpp

wince* {
    DEFINES += SRCDIR=\\\"\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}
