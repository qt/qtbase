CONFIG += qttest_p4

QT = core
TARGET = tst_rcc

SOURCES += tst_rcc.cpp

wince* {
    DEFINES += SRCDIR=\\\"\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

