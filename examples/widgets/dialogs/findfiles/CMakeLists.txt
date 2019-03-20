# Generated from findfiles.pro.

#####################################################################
## findfiles Binary:
#####################################################################

add_qt_executable(findfiles
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/dialogs/findfiles"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/dialogs/findfiles"
    SOURCES
        main.cpp
        window.cpp window.h
    LIBRARIES
        Qt::Widgets
)
