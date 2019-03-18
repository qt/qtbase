# Generated from windows.pro.

#####################################################################
## windows Binary:
#####################################################################

add_qt_executable(windows
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/qpa/windows"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/qpa/windows"
    SOURCES
        main.cpp
        window.cpp window.h
    LIBRARIES
        Qt::GuiPrivate
        Qt::CorePrivate
)

#### Keys ignored in scope 1:.:windows.pro:<NONE>:
# CONFIG = "console"
