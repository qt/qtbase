# Generated from screenshot.pro.

#####################################################################
## screenshot Binary:
#####################################################################

add_qt_executable(screenshot
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/desktop/screenshot"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/desktop/screenshot"
    SOURCES
        main.cpp
        screenshot.cpp screenshot.h
    LIBRARIES
        Qt::Widgets
)
