# Generated from rogue.pro.

#####################################################################
## rogue Binary:
#####################################################################

add_qt_executable(rogue
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/statemachine/rogue"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/statemachine/rogue"
    SOURCES
        main.cpp
        movementtransition.h
        window.cpp window.h
    LIBRARIES
        Qt::Widgets
)
