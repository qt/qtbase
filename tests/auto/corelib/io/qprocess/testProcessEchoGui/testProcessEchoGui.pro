win32 {
   SOURCES = main_win.cpp
   !win32-borland:LIBS += -lUser32
}

CONFIG -= qt app_bundle
DESTDIR = ./
