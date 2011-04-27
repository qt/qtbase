load(qttest_p4)
QT -= gui

SOURCES += tst_qelapsedtimer.cpp
wince* {
    DEFINES += SRCDIR=\\\"\\\"
} else:symbian {
    # do not define SRCDIR at all
    TARGET.EPOCHEAPSIZE = 0x100000 0x3000000
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

CONFIG += parallel_test
