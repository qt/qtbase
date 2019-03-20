# Generated from spinboxes.pro.

#####################################################################
## spinboxes Binary:
#####################################################################

add_qt_executable(spinboxes
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/spinboxes"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/spinboxes"
    SOURCES
        main.cpp
        window.cpp window.h
    LIBRARIES
        Qt::Widgets
)
