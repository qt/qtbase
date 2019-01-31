# Generated from jpeg.pro.

#####################################################################
## qjpeg Plugin:
#####################################################################

find_package(JPEG)

add_qt_plugin(qjpeg
    TYPE imageformats
    SOURCES
        main.cpp main.h
        qjpeghandler.cpp qjpeghandler_p.h
    LIBRARIES
        Qt::Gui
        Qt::CorePrivate
        Qt::GuiPrivate
        JPEG::JPEG
)
