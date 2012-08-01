win32 {
   SOURCES = main_win.cpp
   !win32-borland:LIBS += -luser32
}

CONFIG -= qt app_bundle
DESTDIR = ./
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
