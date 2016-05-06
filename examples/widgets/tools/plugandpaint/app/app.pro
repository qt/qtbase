#! [0]
TARGET = plugandpaint
DESTDIR = ..

QT += widgets

HEADERS        = interfaces.h \
                 mainwindow.h \
                 paintarea.h \
                 plugindialog.h
SOURCES        = main.cpp \
                 mainwindow.cpp \
                 paintarea.cpp \
                 plugindialog.cpp

LIBS           = -L../plugins -lpnp_basictools

if(!debug_and_release|build_pass):CONFIG(debug, debug|release) {
   mac:LIBS = $$member(LIBS, 0) $$member(LIBS, 1)_debug
   win32:LIBS = $$member(LIBS, 0) $$member(LIBS, 1)d
}
#! [0]

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tools/plugandpaint
INSTALLS += target

CONFIG += install_ok  # Do not cargo-cult this!
