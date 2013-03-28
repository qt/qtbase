CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qglyphrun
QT = core gui testlib

linux: CONFIG += insignificant_test

SOURCES += \
    tst_qglyphrun.cpp

wince* {
    DEFINES += SRCDIR=\\\"\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}
