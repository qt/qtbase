TEMPLATE = lib
CONFIG += dll
CONFIG -= staticlib
SOURCES		= mylib.c
TARGET = mylib
DESTDIR = ../
QT = core

wince: DEFINES += WIN32_MSVC
win32-msvc: DEFINES += WIN32_MSVC

# This project is testdata for tst_qlibrary
target.path = $$[QT_INSTALL_TESTS]/tst_qlibrary
INSTALLS += target

win32 {
    CONFIG(debug, debug|release) {
        DESTDIR = ../debug/
    } else {
        DESTDIR = ../release/
    }
}
