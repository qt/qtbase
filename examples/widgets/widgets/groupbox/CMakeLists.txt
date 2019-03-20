# Generated from groupbox.pro.

#####################################################################
## groupbox Binary:
#####################################################################

add_qt_executable(groupbox
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/groupbox"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/groupbox"
    SOURCES
        main.cpp
        window.cpp window.h
    LIBRARIES
        Qt::Widgets
)
