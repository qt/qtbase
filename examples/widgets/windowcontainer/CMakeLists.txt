# Generated from windowcontainer.pro.

#####################################################################
## windowcontainer Binary:
#####################################################################

add_qt_executable(windowcontainer
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/windowcontainer"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/windowcontainer"
    SOURCES
        ../../gui/openglwindow/openglwindow.cpp ../../gui/openglwindow/openglwindow.h
        windowcontainer.cpp
    INCLUDE_DIRECTORIES
        ../../gui/openglwindow
    LIBRARIES
        Qt::Widgets
)
