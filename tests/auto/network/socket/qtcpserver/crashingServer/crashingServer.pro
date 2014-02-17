SOURCES += main.cpp
QT = core network
CONFIG -= app_bundle
DESTDIR = ./

# This means the auto test works on some machines for MinGW. No dialog stalls
# the application.
mingw:CONFIG += console
