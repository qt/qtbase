SOURCES += main.cpp
QT = core network
CONFIG -= app_bundle
DESTDIR = ./

# This means the auto test works on some machines for MinGW. No dialog stalls
# the application.
win32-g++*:CONFIG += console
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
