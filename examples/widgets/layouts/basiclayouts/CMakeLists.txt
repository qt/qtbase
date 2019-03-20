# Generated from basiclayouts.pro.

#####################################################################
## basiclayouts Binary:
#####################################################################

add_qt_executable(basiclayouts
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/layouts/basiclayouts"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/layouts/basiclayouts"
    SOURCES
        dialog.cpp dialog.h
        main.cpp
    LIBRARIES
        Qt::Widgets
)
