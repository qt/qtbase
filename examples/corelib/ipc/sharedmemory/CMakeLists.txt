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
    PUBLIC_LIBRARIES
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)

#### Keys ignored in scope 1:.:.:sharedmemory.pro:<TRUE>:
# EXAMPLE_FILES = "*.png"
