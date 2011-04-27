SOURCES = main.cpp
QT = core
CONFIG += console
CONFIG -= app_bundle
INSTALLS =
DESTDIR = ./

symbian: {
TARGET.EPOCSTACKSIZE =0x14000
}
