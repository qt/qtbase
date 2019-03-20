# Generated from eventtransitions.pro.

#####################################################################
## eventtransitions Binary:
#####################################################################

add_qt_executable(eventtransitions
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/statemachine/eventtransitions"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/statemachine/eventtransitions"
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Widgets
)
