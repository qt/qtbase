
TARGET = QtEntryPoint
MODULE = entrypoint

CONFIG += header_module no_module_headers internal_module

MODULE_DEPENDS = entrypoint_implementation
QT =

mingw {
    MODULE_DEFINES += QT_NEEDS_QMAIN

    # This library needs to come before the entry-point library in the
    # linker line, so that the static linker will pick up the WinMain
    # symbol from the entry-point library.
    MODULE_LDFLAGS += -lmingw32
}

MODULE_PRI_EXTRA_CONTENT = \
    "QT.entrypoint_implementation.name = QtEntryPointImplementation" \
    "QT.entrypoint_implementation.module = Qt6EntryPoint" \
    "QT.entrypoint_implementation.libs = \$\$QT_MODULE_LIB_BASE" \
    "QT.entrypoint_implementation.module_config = staticlib v2 internal_module"

load(qt_module)
