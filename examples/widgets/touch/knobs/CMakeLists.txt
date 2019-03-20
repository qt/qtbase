# Generated from knobs.pro.

#####################################################################
## knobs Binary:
#####################################################################

add_qt_executable(knobs
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/touch/knobs"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/touch/knobs"
    SOURCES
        knob.cpp knob.h
        main.cpp
    LIBRARIES
        Qt::Widgets
)
