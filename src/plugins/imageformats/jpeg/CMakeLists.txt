# Generated from jpeg.pro.

#####################################################################
## qjpeg Plugin:
#####################################################################

qt_find_package(JPEG) # special case

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
