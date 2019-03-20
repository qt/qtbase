# Generated from wiggly.pro.

#####################################################################
## wiggly Binary:
#####################################################################

add_qt_executable(wiggly
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/wiggly"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/wiggly"
    SOURCES
        dialog.cpp dialog.h
        main.cpp
        wigglywidget.cpp wigglywidget.h
    LIBRARIES
        Qt::Widgets
)
