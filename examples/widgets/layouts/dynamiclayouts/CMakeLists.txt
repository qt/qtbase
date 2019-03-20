# Generated from dynamiclayouts.pro.

#####################################################################
## dynamiclayouts Binary:
#####################################################################

add_qt_executable(dynamiclayouts
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/layouts/dynamiclayouts"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/layouts/dynamiclayouts"
    SOURCES
        dialog.cpp dialog.h
        main.cpp
    LIBRARIES
        Qt::Widgets
)
