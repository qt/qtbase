
TARGET = QtEntryPoint
MODULE = entrypoint

CONFIG += header_module no_module_headers internal_module

QT =

win32 {
    MODULE_DEPENDS = entrypoint_implementation

    mingw {
        MODULE_DEFINES += QT_NEEDS_QMAIN

        # This library needs to come before the entry-point library in the
        # linker line, so that the static linker will pick up the WinMain
        # symbol from the entry-point library.
        MODULE_LDFLAGS += -lmingw32
    }
}

uikit {
    # The LC_MAIN load command available in iOS 6.0 and above allows dyld to
    # directly call the entrypoint instead of going through _start in crt.o.
    # Passing -e to the linker changes the entrypoint from _main to our custom
    # wrapper that calls UIApplicationMain and dispatches back to main() once
    # the application has started up and is ready to initialize QApplication.
    MODULE_LDFLAGS += -Wl,-e,_qt_main_wrapper
}

contains(MODULE_DEPENDS, entrypoint_implementation) {
    MODULE_PRI_EXTRA_CONTENT = \
        "QT.entrypoint_implementation.name = QtEntryPointImplementation" \
        "QT.entrypoint_implementation.module = Qt6EntryPoint" \
        "QT.entrypoint_implementation.libs = \$\$QT_MODULE_LIB_BASE" \
        "QT.entrypoint_implementation.module_config = staticlib v2 internal_module"
}

load(qt_module)
