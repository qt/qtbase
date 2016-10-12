TARGET = QtClipboardSupport
MODULE = clipboard_support

QT = core-private gui
CONFIG += static internal_module

DEFINES += QT_NO_CAST_FROM_ASCII
PRECOMPILED_HEADER = ../../corelib/global/qt_pch.h

HEADERS += qmacmime_p.h
SOURCES += qmacmime.mm

macos: LIBS_PRIVATE += -framework AppKit

load(qt_module)
