# Generated from lineedits.pro.

#####################################################################
## lineedits Binary:
#####################################################################

add_qt_executable(lineedits
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/lineedits"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/lineedits"
    SOURCES
        main.cpp
        window.cpp window.h
    LIBRARIES
        Qt::Widgets
)
