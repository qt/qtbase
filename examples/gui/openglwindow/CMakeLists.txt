# Generated from openglwindow.pro.

#####################################################################
## openglwindow Binary:
#####################################################################

add_qt_executable(openglwindow
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/gui/openglwindow"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/gui/openglwindow"
    SOURCES
        main.cpp
        openglwindow.cpp openglwindow.h
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
    LIBRARIES # special case
        Qt::Gui
)
