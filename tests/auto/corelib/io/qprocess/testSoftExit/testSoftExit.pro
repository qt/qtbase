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

# This app is testdata for tst_qprocess
target.path = $$[QT_INSTALL_TESTS]/tst_qprocess/$$TARGET
INSTALLS += target
