# Generated from painterpaths.pro.

#####################################################################
## painterpaths Binary:
#####################################################################

add_qt_executable(painterpaths
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/painting/painterpaths"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/painting/painterpaths"
    SOURCES
        main.cpp
        renderarea.cpp renderarea.h
        window.cpp window.h
    LIBRARIES
        Qt::Widgets
        painting_shared # special case
)

## Scopes:
#####################################################################

extend_target(painterpaths CONDITION UNIX AND NOT APPLE_OSX AND NOT HAIKU AND NOT INTEGRITY AND NOT VXWORKS
    LIBRARIES
        m
)
