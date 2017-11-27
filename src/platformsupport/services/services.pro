TARGET = QtServiceSupport
MODULE = service_support

QT = core-private gui-private
qtConfig(dbus): QT += dbus

CONFIG += static internal_module

DEFINES += QT_NO_CAST_FROM_ASCII
PRECOMPILED_HEADER = ../../corelib/global/qt_pch.h

include($$PWD/genericunix/genericunix.pri)

load(qt_module)
