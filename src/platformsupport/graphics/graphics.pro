TARGET = QtGraphicsSupport
MODULE = graphics_support

QT = core-private gui-private
CONFIG += static internal_module

DEFINES += QT_NO_CAST_FROM_ASCII
PRECOMPILED_HEADER = ../../corelib/global/qt_pch.h

HEADERS += $$PWD/qrasterbackingstore_p.h
SOURCES += $$PWD/qrasterbackingstore.cpp

load(qt_module)
