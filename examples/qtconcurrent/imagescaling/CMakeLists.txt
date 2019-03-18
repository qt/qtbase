# Generated from imagescaling.pro.

#####################################################################
## imagescaling Binary:
#####################################################################

add_qt_executable(imagescaling
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/qtconcurrent/imagescaling"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/qtconcurrent/imagescaling"
    SOURCES
        imagescaling.cpp imagescaling.h
        main.cpp
    LIBRARIES
        Qt::Concurrent
        Qt::Widgets
)
