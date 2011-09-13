TEMPLATE = lib
CONFIG += plugin debug
CONFIG -= release debug_and_release
SOURCES = main.cpp
QT = core
DESTDIR = ../plugins

symbian: {
    TARGET.EPOCALLOWDLLDATA=1
    TARGET.CAPABILITY=ALL -TCB
}
