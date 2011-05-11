load(qttest_p4)
QT = core gui

SOURCES += \
    tst_qglyphrun.cpp

wince*|symbian*: {
    DEFINES += SRCDIR=\\\"\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}
