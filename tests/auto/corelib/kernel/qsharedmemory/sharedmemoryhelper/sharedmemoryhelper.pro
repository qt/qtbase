QT = core testlib

win32: CONFIG += console
mac:CONFIG -= app_bundle

SOURCES += main.cpp
TARGET = helperbinary

CONFIG(debug_and_release) {
    CONFIG(debug, debug|release) {
        DESTDIR = ../debug
    } else {
        DESTDIR = ../release
    }
} else {
    DESTDIR = ..
}
