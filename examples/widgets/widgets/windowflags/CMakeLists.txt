# Generated from windowflags.pro.

#####################################################################
## windowflags Binary:
#####################################################################

add_qt_executable(windowflags
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/windowflags"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/windowflags"
    SOURCES
        controllerwindow.cpp controllerwindow.h
        main.cpp
        previewwindow.cpp previewwindow.h
    LIBRARIES
        Qt::Widgets
)
