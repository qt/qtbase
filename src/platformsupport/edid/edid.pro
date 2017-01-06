TARGET = QtEdidSupport
MODULE = edid_support

QT = core-private
CONFIG += static internal_module

DEFINES += QT_NO_CAST_FROM_ASCII
PRECOMPILED_HEADER = ../../corelib/global/qt_pch.h

HEADERS += qedidparser_p.h
SOURCES += qedidparser.cpp

load(qt_module)
