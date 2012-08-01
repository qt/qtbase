CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qglyphrun
QT = core gui testlib

SOURCES += \
    tst_qglyphrun.cpp

wince* {
    DEFINES += SRCDIR=\\\"\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
