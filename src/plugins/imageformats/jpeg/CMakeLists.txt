# Generated from jpeg.pro.

qt_find_package(JPEG) # special case

#####################################################################
## qjpeg Plugin:
#####################################################################

add_qt_plugin(qjpeg
    TYPE imageformats
    CLASS_NAME QJpegPlugin
    SOURCES
        main.cpp main.h
        qjpeghandler.cpp qjpeghandler_p.h
    LIBRARIES
        # Qt::CorePrivate # special case: remove
        Qt::GuiPrivate
    PUBLIC_LIBRARIES # special case
        JPEG::JPEG
        # Qt::Core # special case: remove
        Qt::Gui # special case
)

#### Keys ignored in scope 1:.:.:jpeg.pro:<TRUE>:
# OTHER_FILES = "jpeg.json"
# PLUGIN_CLASS_NAME = "QJpegPlugin"
# _LOADED = "qt_plugin"
