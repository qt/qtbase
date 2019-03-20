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
    LIBRARIES
        Qt::Widgets
)

## Scopes:
#####################################################################

extend_target(fontsampler CONDITION TARGET Qt::PrintSupport
    LIBRARIES
        Qt::PrintSupport
)
