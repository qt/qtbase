QT += widgets
SOURCES += main.cpp \
    base.cpp
DESTDIR = ./
CONFIG -= app_bundle debug_and_release_target
HEADERS += base.h

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
