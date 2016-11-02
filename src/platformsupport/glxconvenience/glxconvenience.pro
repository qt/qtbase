TARGET = QtGlxSupport
MODULE = glx_support

QT = core-private gui-private
CONFIG += static internal_module

DEFINES += QT_NO_CAST_FROM_ASCII
PRECOMPILED_HEADER = ../../corelib/global/qt_pch.h

qtConfig(xrender): QMAKE_USE_PRIVATE += xrender
LIBS_PRIVATE += $$QMAKE_LIBS_X11

HEADERS += qglxconvenience_p.h
SOURCES += qglxconvenience.cpp

load(qt_module)
