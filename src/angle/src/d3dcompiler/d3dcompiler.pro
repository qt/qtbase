TARGET = d3dcompiler_qt
CONFIG += installed
include(../config.pri)

CONFIG += qt
QT = core
DEFINES += QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII
SOURCES += main.cpp
win32:!winrt: LIBS += -lole32

winrt:equals(WINSDK_VER, 8.1) {
    DEFINES += D3DCOMPILER_LINKED
    LIBS += -ld3dcompiler
}

# __stdcall exports get mangled, so use a def file
DEF_FILE += $${TARGET}.def
