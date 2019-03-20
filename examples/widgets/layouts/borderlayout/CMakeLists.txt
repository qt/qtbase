# Generated from borderlayout.pro.

#####################################################################
## borderlayout Binary:
#####################################################################

add_qt_executable(borderlayout
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/layouts/borderlayout"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/layouts/borderlayout"
    SOURCES
        borderlayout.cpp borderlayout.h
        main.cpp
        window.cpp window.h
    LIBRARIES
        Qt::Widgets
)
