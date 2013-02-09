SOURCES = main.cpp
CONFIG -= qt
CONFIG += console
DESTDIR = "../test Space In Name"

mac {
  CONFIG -= app_bundle
}
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
QT = core
