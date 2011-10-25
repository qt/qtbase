CONFIG += testcase
TARGET = tst_qsound
SOURCES += tst_qsound.cpp
QT += testlib widgets

wince* {
    deploy.files += 4.wav
    DEPLOYMENT += deploy
    DEFINES += SRCDIR=\\\"\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}
