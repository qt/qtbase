# Generated from fontsampler.pro.

#####################################################################
## fontsampler Binary:
#####################################################################

add_qt_executable(fontsampler
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/painting/fontsampler"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/painting/fontsampler"
    SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h
        mainwindowbase.ui
    PUBLIC_LIBRARIES
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)

## Scopes:
#####################################################################

extend_target(fontsampler CONDITION TARGET Qt::PrintSupport
    PUBLIC_LIBRARIES
        Qt::PrintSupport
)
