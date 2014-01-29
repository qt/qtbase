TEMPLATE = lib
TARGET = $$qtLibraryTarget(d3dcompiler_qt)

include(../config.pri)
CONFIG += qt

QT = core
DEFINES += QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII
SOURCES += main.cpp
win32:!winrt: LIBS += -lole32

# __stdcall exports get mangled, so use a def file
DEF_FILE += $${TARGET}.def

load(qt_installs)
