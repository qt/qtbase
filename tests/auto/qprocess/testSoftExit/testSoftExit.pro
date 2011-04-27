win32 {
   SOURCES = main_win.cpp
   !win32-borland:!wince*:LIBS += -lUser32
}
unix {
   SOURCES = main_unix.cpp
}

CONFIG -= qt app_bundle
CONFIG += console
DESTDIR = ./

# no install rule for application used by test
INSTALLS =


