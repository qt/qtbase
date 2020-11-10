
TEMPLATE = lib

TARGET = QtEntryPoint

CONFIG += static no_module_headers internal_module
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
    }
}

# Minimal qt_helper_lib

load(qt_build_paths)
load(qt_common)

qtConfig(debug_and_release): CONFIG += debug_and_release
qtConfig(build_all): CONFIG += build_all

DESTDIR = $$MODULE_BASE_OUTDIR/lib

TARGET = $$qt5LibraryTarget($$TARGET)

load(qt_installs)
