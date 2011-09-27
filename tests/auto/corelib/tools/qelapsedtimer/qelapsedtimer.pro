load(qttest_p4)
QT -= gui

SOURCES += tst_qelapsedtimer.cpp
wince* {
    DEFINES += SRCDIR=\\\"\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

CONFIG += parallel_test
