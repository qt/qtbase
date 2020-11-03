
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
        MODULE_DEFINES += QT_NEEDS_QMAIN

        # This library needs to come before the entry-point library in the
        # linker line, so that the static linker will pick up the WinMain
        # symbol from the entry-point library.
        MODULE_LDFLAGS += -lmingw32
    }
}

# Minimal qt_helper_lib

load(qt_build_paths)
load(qt_common)

!build_pass {
    MODULE_PRI_CONTENT = \
        "QT.entrypoint_implementation.name = QtEntryPointImplementation" \
        "QT.entrypoint_implementation.module = Qt6EntryPoint" \
        "QT.entrypoint_implementation.ldflags = $$MODULE_LDFLAGS" \
        "QT.entrypoint_implementation.libs = \$\$QT_MODULE_LIB_BASE" \
        "QT.entrypoint_implementation.DEFINES = $$MODULE_DEFINES" \
        "QT.entrypoint_implementation.module_config = staticlib v2 internal_module"

    module_path = $$MODULE_QMAKE_OUTDIR/mkspecs/modules
    force_independent|split_incpath: module_path = "$${module_path}-inst"
    MODULE_PRI = $$module_path/qt_lib_entrypoint_private.pri
    write_file($$MODULE_PRI, MODULE_PRI_CONTENT, append)|error()
}

qtConfig(debug_and_release): CONFIG += debug_and_release
qtConfig(build_all): CONFIG += build_all

DESTDIR = $$MODULE_BASE_OUTDIR/lib

TARGET = $$qt5LibraryTarget($$TARGET)

load(qt_installs)
