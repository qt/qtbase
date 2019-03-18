# Generated from sharedmemory.pro.

#####################################################################
## sharedmemory Binary:
#####################################################################

add_qt_executable(sharedmemory
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/corelib/ipc/sharedmemory"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/corelib/ipc/sharedmemory"
    SOURCES
        dialog.cpp dialog.h dialog.ui
        main.cpp
    LIBRARIES
        Qt::Widgets
)

#### Keys ignored in scope 1:.:sharedmemory.pro:<NONE>:
# EXAMPLE_FILES = "*.png"
