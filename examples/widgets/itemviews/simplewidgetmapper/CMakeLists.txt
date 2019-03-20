# Generated from simplewidgetmapper.pro.

#####################################################################
## simplewidgetmapper Binary:
#####################################################################

add_qt_executable(simplewidgetmapper
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/simplewidgetmapper"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/simplewidgetmapper"
    SOURCES
        main.cpp
        window.cpp window.h
    LIBRARIES
        Qt::Widgets
)
