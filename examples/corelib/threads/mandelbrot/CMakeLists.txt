# Generated from mandelbrot.pro.

#####################################################################
## mandelbrot Binary:
#####################################################################

add_qt_executable(mandelbrot
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/corelib/threads/mandelbrot"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/corelib/threads/mandelbrot"
    SOURCES
        main.cpp
        mandelbrotwidget.cpp mandelbrotwidget.h
        renderthread.cpp renderthread.h
    LIBRARIES
        Qt::Widgets
)

## Scopes:
#####################################################################

extend_target(mandelbrot CONDITION UNIX AND NOT APPLE_OSX AND NOT HAIKU AND NOT INTEGRITY AND NOT VXWORKS
    LIBRARIES
        m
)
