CONFIG += testcase
TARGET = tst_qrawfont

QT = core core-private gui gui-private testlib

SOURCES += \
    tst_qrawfont.cpp

INCLUDEPATH += $$QT_SOURCE_TREE/src/3rdparty/harfbuzz/src

wince* {
    DEFINES += SRCDIR=\\\"\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}
