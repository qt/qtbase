load(qttest_p4)
QT = core gui

SOURCES += \
    tst_qrawfont.cpp

INCLUDEPATH += $$QT_SOURCE_TREE/src/3rdparty/harfbuzz/src

wince*|symbian*: {
    DEFINES += SRCDIR=\\\"\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}
