win32 {
   SOURCES = main_win.cpp
   !win32-borland:!wince*:LIBS += -luser32
}
unix {
   SOURCES = main_unix.cpp
}

CONFIG -= qt app_bundle
CONFIG += console
DESTDIR = ./
