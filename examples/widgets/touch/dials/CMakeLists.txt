# Generated from dials.pro.

#####################################################################
## dials Binary:
#####################################################################

add_qt_executable(dials
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/touch/dials"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/touch/dials"
    SOURCES
        dials.ui
        main.cpp
    LIBRARIES
        Qt::Widgets
)
