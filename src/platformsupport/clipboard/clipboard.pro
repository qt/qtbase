TARGET = QtClipboardSupport
MODULE = clipboard_support

QT = core-private gui
CONFIG += static internal_module

DEFINES += QT_NO_CAST_FROM_ASCII

HEADERS += qmacmime_p.h
SOURCES += qmacmime.mm

LIBS += -framework ImageIO
macos: LIBS_PRIVATE += -framework AppKit

load(qt_module)
