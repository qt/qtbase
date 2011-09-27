load(qttest_p4)
QT = core gui

SOURCES += \
    tst_qglyphrun.cpp

wince* {
    DEFINES += SRCDIR=\\\"\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}
