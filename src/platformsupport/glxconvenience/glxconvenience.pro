TARGET = QtGlxSupport
MODULE = glx_support

QT = core-private gui-private
CONFIG += static internal_module

DEFINES += QT_NO_CAST_FROM_ASCII

QMAKE_USE += xlib

HEADERS += qglxconvenience_p.h
SOURCES += qglxconvenience.cpp

load(qt_module)
