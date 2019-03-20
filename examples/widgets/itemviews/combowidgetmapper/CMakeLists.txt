# Generated from combowidgetmapper.pro.

#####################################################################
## combowidgetmapper Binary:
#####################################################################

add_qt_executable(combowidgetmapper
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/combowidgetmapper"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/combowidgetmapper"
    SOURCES
        main.cpp
        window.cpp window.h
    LIBRARIES
        Qt::Widgets
)
