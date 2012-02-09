CONFIG += testcase
TARGET = tst_qglyphrun
QT = core gui testlib

SOURCES += \
    tst_qglyphrun.cpp

wince* {
    DEFINES += SRCDIR=\\\"\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

win32:CONFIG += insignificant_test # QTBUG-24196
