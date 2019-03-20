# Generated from trafficlight.pro.

#####################################################################
## trafficlight Binary:
#####################################################################

add_qt_executable(trafficlight
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/statemachine/trafficlight"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/statemachine/trafficlight"
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Widgets
)
