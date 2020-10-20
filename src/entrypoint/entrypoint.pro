# Additional Qt project file for QtEntryPoint lib
!win32:error("$$_FILE_ is intended only for Windows!")

TARGET = QtEntryPoint

CONFIG += static no_module_headers
QT = core

DEFINES += QT_NO_FOREACH

win32 {
    SOURCES = qtentrypoint_win.cpp
    CONFIG -= qt

    QMAKE_USE_PRIVATE += shell32

    msvc {
        QMAKE_CFLAGS_DEBUG -= -Zi
        QMAKE_CXXFLAGS_DEBUG -= -Zi
        QMAKE_CFLAGS_DEBUG *= -Z7
        QMAKE_CXXFLAGS_DEBUG *= -Z7
    }
    mingw {
        DEFINES += QT_NEEDS_QMAIN
        MODULE_DEFINES += QT_NEEDS_QMAIN
    }
}

load(qt_module)
