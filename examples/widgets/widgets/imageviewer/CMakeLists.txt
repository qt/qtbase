# Generated from imageviewer.pro.

#####################################################################
## imageviewer Binary:
#####################################################################

add_qt_executable(imageviewer
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/imageviewer"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/imageviewer"
    SOURCES
        imageviewer.cpp imageviewer.h
        main.cpp
    LIBRARIES
        Qt::Widgets
)

## Scopes:
#####################################################################

extend_target(imageviewer CONDITION TARGET Qt::PrintSupport
    LIBRARIES
        Qt::PrintSupport
)
