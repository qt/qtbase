TARGET = QtAccessibilitySupport
MODULE = accessibility_support

QT = core-private gui-private
CONFIG += static internal_module

DEFINES += QT_NO_CAST_FROM_ASCII

HEADERS += \
    qaccessiblebridgeutils_p.h

SOURCES += \
    qaccessiblebridgeutils.cpp

load(qt_module)
