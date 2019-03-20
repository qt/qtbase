# Generated from moveblocks.pro.

#####################################################################
## moveblocks Binary:
#####################################################################

add_qt_executable(moveblocks
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/animation/moveblocks"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/animation/moveblocks"
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Widgets
)
