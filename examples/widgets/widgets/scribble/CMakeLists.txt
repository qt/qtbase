# Generated from scribble.pro.

#####################################################################
## scribble Binary:
#####################################################################

add_qt_executable(scribble
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/scribble"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/scribble"
    SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h
        scribblearea.cpp scribblearea.h
    LIBRARIES
        Qt::Widgets
)

## Scopes:
#####################################################################

extend_target(scribble CONDITION TARGET Qt::PrintSupport
    LIBRARIES
        Qt::PrintSupport
)
